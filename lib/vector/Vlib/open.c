/*!
  \file lib/vector/Vlib/open.c
  
  \brief Vector library - Open existing or create new vector map
  (native or OGR/PostGIS format)
  
  Higher level functions for reading/writing/manipulating vectors.
  
  (C) 2001-2009, 2012-2013 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL, probably Dave Gerdes or Mike Higgins.
  \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
  \author Update to GRASS 7 Martin Landa <landa.martin gmail.com> (better OGR support and native PostGIS access)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"
#endif

/*
  \brief Number of levels

   - 1 without topology
   - 2 with 2D topology

   \todo Implement
   - 3 with 3D topology
*/
#define MAX_OPEN_LEVEL 2

static int open_old_dummy()
{
    return 0;
}

static int open_new_dummy()
{
    return 0;
}

#if !defined HAVE_OGR || !defined HAVE_POSTGRES
static int format()
{
    G_fatal_error(_("Requested format is not compiled in this version"));
    return 0;
}
#endif

static int Open_level = 0;

static int (*Open_old_array[][2]) () = {
    {
    open_old_dummy, V1_open_old_nat}
#ifdef HAVE_OGR
    , {
    open_old_dummy, V1_open_old_ogr}
    , {
    open_old_dummy, V1_open_old_ogr}
#else
    , {
    open_old_dummy, format}
    , {
    open_old_dummy, format}
#endif
#ifdef HAVE_POSTGRES
    , {
    open_old_dummy, V1_open_old_pg}
#else
    , {
    open_old_dummy, format}
#endif
};

static int (*Open_new_array[][2]) () = {
    {
    open_new_dummy, V1_open_new_nat}
#ifdef HAVE_OGR
    , {
    open_new_dummy, V1_open_new_ogr}
    , {
    open_new_dummy, V1_open_new_ogr}
#else
    , {
    open_new_dummy, format}
    , {
    open_new_dummy, format}
#endif
#ifdef HAVE_POSTGRES
    , {
    open_old_dummy, V1_open_new_pg}
#else
    , {
    open_old_dummy, format}
#endif
};

static int open_new(struct Map_info *, const char *, int, int);
static int map_format(struct Map_info *);

/*!
  \brief Predetermine level at which a vector map will be opened for
  reading.
  
  If it can't open that level, the open will fail. The specified level
  must be set before any call to open. The default is to try to open
  the highest level possible, and keep stepping down until success.
  
  NOTE: This should only be used to set when you wish to force a lower
  level open. If you require a higher level, then just check the
  return to verify the level instead of forcing it. This is because
  future releases will have higher levels which will be downward
  compatible and which your programs should support by default.
 
  \param level vector access level
  
  \return 0 on success
  \return 1 on error (invalid access level)
*/
int Vect_set_open_level(int level)
{
    Open_level = level;
    if (Open_level < 1 || Open_level > MAX_OPEN_LEVEL) {
        G_warning(_("Programmer requested unknown access level %d"),
                  Open_level);
        Open_level = 0;
        return 1;
    }

    return 0;
}

/*! 
 \brief Open existing vector map for reading (internal use only)
 
 \param[out] Map pointer to Map_info structure
 \param name name of vector map to open
 \param mapset mapset name ("" for search path)
 \param layer layer name (OGR format only)
 \param update non-zero to open for update otherwise read-only mode
 \param head_only read only header info from 'head', 'dbln', 'topo',
 'cidx' is not opened. The header may be opened on level 2 only.
 \param is_tmp TRUE for temporary maps

 \return level of openness (1, 2)
 \return -1 in error
*/
int Vect__open_old(struct Map_info *Map, const char *name, const char *mapset,
                   const char *layer, int update, int head_only, int is_tmp)
{
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    char *path;
    FILE *fp;
    int level, level_request;
    int format, ret;
    int ogr_mapset;
    const char *fmapset;

    G_debug(1, "Vect__open_old(): name = %s, mapset = %s, layer = %s, update = %d, "
            "head_only = %d, is_tmp = %d", name, mapset, layer ? layer : "", update, head_only,
            is_tmp);

    /* zero Map_info structure */
    G_zero(Map, sizeof(struct Map_info));

    /* TODO: Open header for update ('dbln') */

    level_request = Open_level;
    Open_level = 0;

    /* initialize Map->head */
    Vect__init_head(Map);
    /* initialize support structures for 2D, update to 3D when reading
       support files */
    Map->plus.spidx_with_z = Map->plus.with_z = Map->head.with_z = WITHOUT_Z;
    /* initialize Map->plus */
    dig_init_plus(&(Map->plus));

    /* check OGR mapset */
    ogr_mapset = FALSE;
    if (G_name_is_fully_qualified(name, xname, xmapset)) {
        if (strcasecmp(xmapset, "ogr") == 0) {
            /* unique OGR mapset detected */
            G_debug(1, "OGR mapset detected");
            ogr_mapset = TRUE;
            Map->fInfo.ogr.dsn = G_store(xname);
            if (layer) {
                Map->fInfo.ogr.layer_name = G_store(layer); /* no layer to be open */
            }
        }
        Map->name = G_store(xname);
        Map->mapset = G_store(xmapset);
    }
    else {
        Map->name = G_store(name);

        Map->temporary = is_tmp;
        /* temporary maps can be accessed only in the current mapset */
        if (mapset)
            Map->mapset = G_store(mapset);
        else
            Map->mapset = G_store("");
    }

    path = Vect__get_path(Map);

    if (!ogr_mapset) {
        /* try to find vector map (not for OGR mapset) */
        if (!Map->temporary) {
            fmapset = G_find_vector2(Map->name, Map->mapset);
            if (fmapset == NULL) {
                if (mapset && strcmp(mapset, G_mapset()) == 0)
                    G_fatal_error(_("Vector map <%s> not found in current mapset"),
                                  Vect_get_name(Map));
                else
                    G_fatal_error(_("Vector map <%s> not found"),
                                  Vect_get_full_name(Map));
                return -1;
            }
            Map->mapset = G_store(fmapset);
        }
        else {
            char file_path[GPATH_MAX];
            
            if (strcmp(Map->mapset, G_mapset()) != 0) {
                G_warning(_("Temporary vector maps can be accessed only in the current mapset"));
                return -1;
            }
            G_file_name(file_path, path, GV_HEAD_ELEMENT, Map->mapset);
            if (access(file_path, F_OK) != 0)
                return -1;
        }
    }
    
    Map->location = G_store(G_location());
    Map->gisdbase = G_store(G_gisdbase());
    
    if (update && !ogr_mapset && (0 != strcmp(Map->mapset, G_mapset()))) {
        G_warning(_("Vector map which is not in the current mapset cannot be opened for update"));
        return -1;
    }

    G_debug(1, "Map: name = %s, mapset = %s, temporary = %d", Map->name, Map->mapset,
            Map->temporary);

    /* read vector format information */
    if (ogr_mapset) {
        format = GV_FORMAT_OGR_DIRECT;
    }
    else {
        format = 0;
        fp = G_fopen_old(path, GV_FRMT_ELEMENT, Map->mapset);
        if (fp == NULL) {
            G_debug(1, "Vector format: %d (native)", format);
            format = GV_FORMAT_NATIVE;
        }
        else {
            format = dig_read_frmt_ascii(fp, &(Map->fInfo));
            fclose(fp);
            
            G_debug(1, "Vector format: %d (non-native)", format);
            if (format < 0) {
                G_fatal_error(_("Unable to open vector map <%s>"),
                              Vect_get_full_name(Map));
                return -1;
            }
        }
    }
    Map->format = format;

    /* read vector head (ignored for OGR mapset) */
    if (!ogr_mapset && Vect__read_head(Map) != 0) {
        G_fatal_error(_("Unable to read header file of vector map <%s>"),
                      Vect_get_full_name(Map));
    }

    /* projection is not written to head but zone ??? */
    if (Vect_get_zone(Map) == -1)
        Vect_set_zone(Map, G_zone());
    Vect_set_proj(Map, G_projection());
    
    G_debug(1, "Level request = %d", level_request);

    /* There are only 2 possible open levels, 1 and 2. Try first to
       open 'support' files (topo, sidx, cidx), these files are the same
       for all formats.  If it is not possible and requested level is
       2, return error, otherwise call Open_old_array[format][1], to
       open remaining files/sources (level 1)
    */

    /* try to open support files if level was not requested or
     * requested level is 2 (format independent) */
    if (level_request == 0 || level_request > 1) {
        level = 2;              /* we expect success */
        
        /* open topo */
        ret = -1;
        if (Map->format == GV_FORMAT_POSTGIS)
            /* try to read full-topology for PostGIS links */
            ret = Vect_open_topo_pg(Map, head_only);
        
        if (ret != 0) {
            /* read topology for native format
               read pseudo-topology for OGR/PostGIS links */
            ret = Vect_open_topo(Map, head_only);    
            
            if (ret == 1) { /* topo file is not available */
                G_debug(1, "topo file for vector '%s' not available.",
                        Vect_get_full_name(Map));
                level = 1;
            }
            else if (ret == -1) {
                G_fatal_error(_("Unable to open topology file for vector map <%s>"),
                              Vect_get_full_name(Map));
            }
        }

        /* open spatial index */
        if (level >= 2) {
            ret = Vect_open_sidx(Map, (update != 0));
            if (ret == 1) {     /* sidx file is not available */
                G_debug(1, "sidx file for vector '%s' not available.",
                        Vect_get_full_name(Map));
                level = 1;
            }
            else if (ret == -1) {
                G_fatal_error(_("Unable to open spatial index file for vector map <%s>"),
                              Vect_get_full_name(Map));
            }
            /* check with_z consistency */
            if ((Map->plus.with_z != 0 && Map->plus.spidx_with_z == 0) ||
                (Map->plus.with_z == 0 && Map->plus.spidx_with_z != 0)) {
                    G_warning("Vector map <%s>: topology is %s, but spatial index is %s",
                    Vect_get_full_name(Map), (Map->plus.with_z != 0 ? "3D" : "2D"),
                    (Map->plus.spidx_with_z != 0 ? "3D" : "2D"));
                    level = 1;
                }
        }
        /* open category index */
        if (level >= 2) {
            ret = Vect_cidx_open(Map, head_only);
            if (ret == 1) {     /* category index is not available */
                G_debug(1,
                        "cidx file for vector '%s' not available.",
                        Vect_get_full_name(Map));
                dig_free_plus(&(Map->plus));    /* free topology */
                level = 1;
            }
            else if (ret == -1) {       /* file exists, but cannot be opened */
                G_fatal_error(_("Unable to open category index file for vector map <%s>"),
                              Vect_get_full_name(Map));
            }
        }
#ifdef HAVE_OGR
        /* open OGR specific support files */
        if (level == 2 && Map->format == GV_FORMAT_OGR) {
            if (V2_open_old_ogr(Map) < 0) {
                dig_free_plus(&(Map->plus));
                level = 1;
            }
        }
#endif
#ifdef HAVE_POSTGRES
        /* open OGR (pseudo-topology access only) specific support
         * files */
        if (level == 2 && Map->format == GV_FORMAT_POSTGIS) {
            if (V2_open_old_pg(Map) < 0) {
                dig_free_plus(&(Map->plus));
                level = 1;
            }
        }
#endif
        if (level_request == 2 && level < 2) {
            if (!ogr_mapset) {
                /* for direct OGR read access is built pseudo-topology on the fly */
                G_warning(_("Unable to open vector map <%s> on level %d. "
                            "Try to rebuild vector topology by v.build."),
                          Vect_get_full_name(Map), level_request);
                return -1;
            }
        }
    }
    else {
        level = 1;              /* i.e. requested level is 1 */
    }

    /* open level 1 files / sources (format specific) */
    if (!head_only || ogr_mapset || format == GV_FORMAT_POSTGIS) {
        /* no need to open coordinates */
        if (0 != (*Open_old_array[format][1]) (Map, update)) {  /* cannot open */
            if (level >= 2) {   /* support files opened */
                dig_free_plus(&(Map->plus));
            }
            if (level_request == 0)
                G_fatal_error(_("Unable to open vector map <%s>"),
                              Vect_get_full_name(Map));
            else
                G_fatal_error(_("Unable to open vector map <%s> on level %d. "
                                "Try to rebuild vector topology by v.build."),
                              Vect_get_full_name(Map), level_request);
            return -1;
        }
        if (ogr_mapset && !head_only && level_request != 1) {
            /* build pseudo-topology on the fly */
            int verbose;
            verbose = G_verbose();
            G_message(_("Building topology for OGR layer <%s> from datasource '%s'..."),
                      Map->fInfo.ogr.layer_name, Map->fInfo.ogr.dsn);
            G_set_verbose(0);
            if (Vect_build(Map)) {
                level = 2;
            }
            G_set_verbose(verbose);
            if (level < level_request)
              G_fatal_error(_("Unable to open vector map <%s> on level %d"),
                            Map->fInfo.ogr.layer_name, level_request);
        }
	if (level < 2 && Map->head.with_z) {
	    /* topo has been initialized as 2D, update to 3D */
	    dig_free_plus(&(Map->plus));
	    
	    Map->plus.with_z = Map->head.with_z;
	    dig_init_plus(&(Map->plus));
	}
    }
    else if (level > 1) {
	/* take dimension from topo if topo is available */
	Map->head.with_z = Map->plus.with_z;
    }

    /* set status */
    Map->open = VECT_OPEN_CODE;
    Map->level = level;
    Map->head_only = head_only;
    Map->support_updated = FALSE;
    if (update) {
        Map->mode = GV_MODE_RW;
        Map->plus.mode = GV_MODE_RW;
    }
    else {
        Map->mode = GV_MODE_READ;
        Map->plus.mode = GV_MODE_READ;
    }
    if (head_only) {
        Map->head_only = TRUE;
    }
    else {
        Map->head_only = FALSE;
    }

    G_debug(1, "Vect__open_old(): vector opened on level %d", level);

    if (level == 1) {           /* without topology */
        Map->plus.built = GV_BUILD_NONE;
    }
    else {                      /* level 2, with topology */
        Map->plus.built = GV_BUILD_ALL; /* highest level of topology for level 2 */
    }

    Map->plus.uplist.do_uplist = FALSE;

    /* read db links */
    Map->dblnk = Vect_new_dblinks_struct();
    Vect_read_dblinks(Map);

    /* open history file */
    if (update && !ogr_mapset) {                /* native only */
        Map->hist_fp = G_fopen_modify(path, GV_HIST_ELEMENT);
        if (Map->hist_fp == NULL) {
            G_warning(_("Unable to open history file for vector map <%s>"),
                      Vect_get_full_name(Map));
            return -1;
        }
        G_fseek(Map->hist_fp, (off_t) 0, SEEK_END);
        Vect_hist_write(Map,
                        "---------------------------------------------------------------------------------\n");

    }
    else {
        if (Map->format == GV_FORMAT_NATIVE || Map->format == GV_FORMAT_OGR ||
            Map->format == GV_FORMAT_POSTGIS) {
            Map->hist_fp = G_fopen_old(path, GV_HIST_ELEMENT, Map->mapset);
            /* If NULL (does not exist) then Vect_hist_read() handle that */
        }
        else {
            Map->hist_fp = NULL;
        }
    }

    if (!head_only) {           /* cannot rewind if not fully opened */
        Vect_rewind(Map);
    }
    
    /* delete support files if native format was opened for update (not head_only) */
    if (update && !head_only) {
        char file_path[GPATH_MAX];

        G_file_name(file_path, path, GV_TOPO_ELEMENT, G_mapset());
        if (access(file_path, F_OK) == 0)       /* topo file exists? */
            unlink(file_path);

        G_file_name(file_path, path, GV_SIDX_ELEMENT, G_mapset());
        if (access(file_path, F_OK) == 0)       /* sidx file exists? */
            unlink(file_path);

        G_file_name(file_path, path, GV_CIDX_ELEMENT, G_mapset());
        if (access(file_path, F_OK) == 0)       /* cidx file exists? */
            unlink(file_path);

        if (format == GV_FORMAT_OGR || format == GV_FORMAT_POSTGIS) {
            G_file_name(file_path, path, GV_FIDX_ELEMENT, G_mapset());
            if (access(file_path, F_OK) == 0)   /* fidx file exists? */
                unlink(file_path);
        }
    }
    G_free(path);
    
    return level;
}

/*!
  \brief Open existing vector map for reading
  
  This function is replaced by Vect_open_old2() to handle also direct
  OGR support.
  
  Calls G_fatal_error() on failure.
  
  \param[out] Map pointer to Map_info structure
  \param name name of vector map to open
  \param mapset mapset name ("" for search path)
  
  \return 1 open on level 1 (without topology)
  \return 2 open on level 2 (with topology)
  \return -1 on error
*/
int Vect_open_old(struct Map_info *Map, const char *name, const char *mapset)
{
    return Vect__open_old(Map, name, mapset, NULL, FALSE, FALSE, FALSE);
}

/*!
  \brief Open existing vector map for reading
  
  Calls G_fatal_error() on failure.

  \param[out] Map pointer to Map_info structure
  \param name name of vector map to open (datasource for direct OGR access)
  \param mapset mapset name ("" for search path, "OGR" for direct OGR access)
  \param layer layer name (OGR layer for direct OGR access)
  
  \return 1 open on level 1 (without topology)
  \return 2 open on level 2 (with topology)
  \return -1 on error
*/
int Vect_open_old2(struct Map_info *Map, const char *name, const char *mapset,
                   const char *layer)
{
    return Vect__open_old(Map, name, mapset, layer, FALSE, FALSE, FALSE);
}

/*!
  \brief Open existing vector map for reading/writing
  
  This function is replaced by Vect_open_update2() to handle also
  direct OGR support.
  
  By default list of updated features is not maintained, see
  Vect_set_updated() for details.
  
  Calls G_fatal_error() on failure.

  \param[out] Map pointer to Map_info structure
  \param name name of vector map to update
  \param mapset mapset name
  
  \return 1 open on level 1 (without topology)
  \return 2 open on level 2 (with topology)
  \return -1 on error
*/
int Vect_open_update(struct Map_info *Map, const char *name, const char *mapset)
{
    return Vect__open_old(Map, name, mapset, NULL, TRUE, FALSE, FALSE);
}

/*!
  \brief Open existing vector map for reading/writing
  
  By default list of updated features is not maintained, see
  Vect_set_updated() for details.
  
  Calls G_fatal_error() on failure.
    
  \param[out] Map pointer to Map_info structure
  \param name name of vector map to open (datasource for direct OGR access)
  \param mapset mapset name ("" for search path, "OGR" for direct OGR access)
  \param layer layer name (OGR layer for direct OGR access)
  
  \return 1 open on level 1 (without topology)
  \return 2 open on level 2 (with topology)
  \return -1 on error 
*/
int Vect_open_update2(struct Map_info *Map, const char *name, const char *mapset, const char *layer)
{
    return Vect__open_old(Map, name, mapset, layer, TRUE, FALSE, FALSE);
}

/*! 
  \brief Reads only info about vector map (headers)

  Reads from headers of 'head', 'dbln', 'topo' and 'cidx' file.
  
  This function is replaced by Vect_open_old_head2() to handle also
  direct OGR support.
  
  Calls G_fatal_error() on failure.
   
  \param[out] Map pointer to Map_info structure
  \param name name of vector map to read
  \param mapset mapset name ("" for search path)
  
  \return 1 open on level 1 (without topology)
  \return 2 open on level 2 (with topology)
  \return -1 on error 
*/
int Vect_open_old_head(struct Map_info *Map, const char *name, const char *mapset)
{
    return Vect__open_old(Map, name, mapset, NULL, FALSE, TRUE, FALSE);
}

/*! 
  \brief Reads only info about vector map (headers)
  
  Reads from headers of 'head', 'dbln', 'topo' and 'cidx' file.
  
  Calls G_fatal_error() on failure.
   
  \param[out] Map pointer to Map_info structure
  \param name name of vector map to read (dsn for OGR)
  \param mapset mapset name ("" for search path)
  \param layer layer name (OGR format)
  
  \param[out] Map pointer to Map_info structure
  \param name name of vector map to open (datasource for direct OGR access)
  \param mapset mapset name ("" for search path, "OGR" for direct OGR access)
  \param layer layer name (OGR layer for direct OGR access)
  
  \return 1 open on level 1 (without topology)
  \return 2 open on level 2 (with topology)
  \return -1 on error 
*/
int Vect_open_old_head2(struct Map_info *Map, const char *name, const char *mapset,
                        const char *layer)
{
    return Vect__open_old(Map, name, mapset, layer, FALSE, TRUE, FALSE);
}

/*!  \brief Open header file of existing vector map for updating
  (mostly for database link updates)
  
  \param[out] Map pointer to Map_info structure
  \param name name of vector map to update
  \param mapset mapset name
  
  \return 1 open on level 1 (without topology)
  \return 2 open on level 2 (with topology)
  \return -1 on error 
*/
int Vect_open_update_head(struct Map_info *Map, const char *name,
                          const char *mapset)
{
    return Vect__open_old(Map, name, mapset, NULL, TRUE, TRUE, FALSE);
}

int open_new(struct Map_info *Map, const char *name, int with_z, int is_tmp)
{
    int ret;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    G_debug(1, "Vect_open_new(): name = %s with_z = %d is_tmp = %d",
            name, with_z, is_tmp);

    /* zero Map_info structure */
    G_zero(Map, sizeof(struct Map_info));
    
    /* init header info */
    Vect__init_head(Map);

    /* check for fully-qualified map name */
    if (G_name_is_fully_qualified(name, xname, xmapset)) {
        if (strcmp(xmapset, G_mapset()) != 0) {
            G_warning(_("Unable to create new vector map: <%s> is not the current mapset (%s)"),
                      name, G_mapset());
            return -1;
        }
        name = xname;
    }

    /* check for [A-Za-z][A-Za-z0-9_]* in name */
    if (Vect_legal_filename(name) < 0) {
        G_warning(_("Unable to create vector map <%s>: name is not SQL compliant"),
            name);
        return -1;
    }

    /* store basic info */
    Map->name      = G_store(name);
    Map->mapset    = G_store(G_mapset());
    Map->location  = G_store(G_location());
    Map->gisdbase  = G_store(G_gisdbase());
    Map->temporary = is_tmp;
    
    /* determine output format */
    if (strcmp(G_program_name(), "v.external") != 0)
        Map->format = map_format(Map);
    else
        Map->format = GV_FORMAT_NATIVE;

    if (Map->format != GV_FORMAT_OGR_DIRECT &&
        getenv("GRASS_VECTOR_PGFILE") == NULL) { /* GRASS_VECTOR_PGFILE defined by v.out.postgis */
        char *path;
        
        G_debug(2, " using non-direct format");

        /* check if map already exists
           temporary maps are automatically overwritten
         */
        if (Map->temporary) {
            if (-1 == Vect__delete(name, Map->temporary)) {
                G_warning(_("Unable to delete vector map <%s>"), name);
                return -1;
            }
        }
        else {
            if (G_find_vector2(name, G_mapset()) != NULL) {
                G_warning(_("Vector map <%s> already exists and will be overwritten"),
                          name);
                
                ret = Vect_delete(name);
                if (ret == -1) {
                    G_warning(_("Unable to delete vector map <%s>"), name);
                    return -1;
                }
            }
        }

        /* write header file

           note: header & history file is also written for external
           formats since vector library create links automatically
           when closing the map
        */
        Map->head.size = 0;
        Map->head.head_size = GV_COOR_HEAD_SIZE + 4;
        Vect__write_head(Map);

        /* create history file */
        path = Vect__get_path(Map);
        Map->hist_fp = G_fopen_new(path, GV_HIST_ELEMENT);
        G_free(path);
        if (Map->hist_fp == NULL) {
            G_warning(_("Unable to open history file of vector map <%s>"),
                      name);
            return -1;
        }
    }

    /* set 2D/3D */
    Map->plus.spidx_with_z = Map->plus.with_z = Map->head.with_z = (with_z != 0);

    Map->level = LEVEL_1;
    
    if ((*Open_new_array[Map->format][1]) (Map, name, with_z) < 0) {
        if (getenv("GRASS_VECTOR_PGFILE") == NULL)  /* GRASS_VECTOR_PGFILE defined by v.out.postgis */
            Vect_delete(name); /* clean up */
        return -1;
    }

    Open_level = 0;

    /* initialize topo */
    Map->plus.Spidx_file = 0;
    dig_init_plus(&(Map->plus));

    /* open new spatial index */
    Vect_open_sidx(Map, 2);

    Map->open = VECT_OPEN_CODE;
    Map->head_only = FALSE;
    Map->support_updated = FALSE;
    Map->plus.built = GV_BUILD_NONE;
    Map->mode = GV_MODE_RW;
    Map->plus.uplist.do_uplist = FALSE;

    Vect_set_proj(Map, G_projection());
    Vect_set_zone(Map, G_zone());

    Map->dblnk = Vect_new_dblinks_struct();

    return 1;
}

/*!
  \brief Create new vector map for reading/writing
 
  By default list of updated features is not maintained, see
  Vect_set_updated() for details.
  
  By default map format is native (GV_FORMAT_NATIVE). If OGR file is
  found in the current mapset then the map (ie. OGR layer) is created
  in given OGR datasource (GV_FORMAT_OGR). Similarly if PG file exists
  then the map (ie. PostGIS table) is created using PostGIS interface
  (GV_FORMAT_POSTGIS). The format of map is stored in Map->format.

  \param[out] Map pointer to Map_info structure
  \param name name of vector map to be created
  \param with_z WITH_Z for 3D vector data otherwise WITHOUT_Z
  
  \return 1 on success
  \return -1 on error
*/
int Vect_open_new(struct Map_info *Map, const char *name, int with_z)
{
    return open_new(Map, name, with_z, FALSE);
}

/*!
  \brief Create new temporary vector map

  Temporary vector maps are stored in the current mapset (directory
  <tt>.tmp/<hostname>/vector</tt>). If the map already exists, it is
  overwritten.

  Temporary vector maps are automatically deleted when closing the map
  (see Vect_close() for details).

  If <em>name</em> is not given (is NULL), then the name is determined
  by process id (<tt>tmp_<pid></tt>).

  \param[out] Map pointer to output Map_info struct
  \param name name for new vector map (or NULL)
  \param with_z WITH_Z for 3D vector data

  \return 1 on success
  \return -1 on error
*/
int Vect_open_tmp_new(struct Map_info *Map, const char *name, int with_z)
{
    char tmp_name[GNAME_MAX];

    if (!name) {
        sprintf(tmp_name, "tmp_%d", getpid());
    }
    else {
        sprintf(tmp_name, "%s", name);
    }
    
    return open_new(Map, tmp_name, with_z, TRUE); /* temporary map */
}

/*!
  \brief Update Coor_info structure
  
  \param Map pointer to Map_info structure
  \param[out] Info pointer to Coor_info structure
  
  \return 1 on success
  \return 0 on error
*/
int Vect_coor_info(const struct Map_info *Map, struct Coor_info *Info)
{
    char *path, file_path[GPATH_MAX];
    STRUCT_STAT stat_buf;
    
    switch (Map->format) {
    case GV_FORMAT_NATIVE:
        path = Vect__get_path(Map);
        G_file_name(file_path, path, GV_COOR_ELEMENT, Map->mapset);
        G_free(path);
        G_debug(1, "get coor info: %s", file_path);
        if (0 != stat(file_path, &stat_buf)) {
            G_warning(_("Unable to stat file <%s>"), file_path);
            Info->size = -1L;
            Info->mtime = -1L;
        }
        else {
            Info->size = (off_t)stat_buf.st_size;      /* file size */
            Info->mtime = (long)stat_buf.st_mtime;     /* last modified time */
        }
        
        /* stat does not give correct size on MINGW
         * if the file is opened */
#ifdef __MINGW32__
        if (Map->open == VECT_OPEN_CODE) {
            dig_fseek(&(Map->dig_fp), 0L, SEEK_END);
            G_debug(2, "dig_ftell = %d", dig_ftell(&(Map->dig_fp)));
            Info->size = dig_ftell(&(Map->dig_fp));
        }
#endif
        break;
    case GV_FORMAT_OGR:
    case GV_FORMAT_OGR_DIRECT:
    case GV_FORMAT_POSTGIS:
        Info->size = 0L;
        Info->mtime = 0L;
        break;
    }
    G_debug(1, "Vect_coor_info(): Info->size = %lu, Info->mtime = %ld",
            (unsigned long)Info->size, Info->mtime);

    return 1;
}

/*!
  \brief Gets vector map format (as string)
  
  Note: string is allocated by G_store(). Free allocated memory with
  G_free().
  
  Currently are implemeted:
   - Native format  (native)
   - OGR format     (ogr)
   - PostGIS format (postgis)
   
   \param Map pointer to Map_info structure
   
   \return maptype string on success (allocated by G_store())
   \return error message on error
*/
const char *Vect_maptype_info(const struct Map_info *Map)
{
    char maptype[1000];

    switch (Map->format) {
    case GV_FORMAT_NATIVE:
        sprintf(maptype, "native");
        break;
    case GV_FORMAT_OGR:
    case GV_FORMAT_OGR_DIRECT:
        sprintf(maptype, "OGR");
        break;
    case GV_FORMAT_POSTGIS:
        sprintf(maptype, "PostGIS");
        break;
    default:
        sprintf(maptype, _("unknown %d (update Vect_maptype_info)"),
                Map->format);
    }

    return G_store(maptype);
}

/*!
  \brief Gets vector map format

  Currently are implemeted:
   - Native format                    (GV_FORMAT_NATIVE)
   - OGR format linked via v.external (GV_FORMAT_OGR)
   - OGR format                       (GV_FORMAT_OGR_DIRECT)
   - PostGIS fomat                    (GV_FORMAT_POSTGIS)
   
  \param Map pointer to Map_info structure
  
  \return map format code
*/
int Vect_maptype(const struct Map_info *Map)
{
    if (Map->temporary) {
        const struct Format_info *finfo;
        
        finfo = &(Map->fInfo);
        if (finfo->ogr.driver_name) {
            return GV_FORMAT_OGR;
        }
        if (finfo->pg.conninfo) {
            return GV_FORMAT_POSTGIS;
        }
    }
    
    return Map->format;
}

/*!
  \brief Open topology file ('topo')
  
  \param[in,out] Map pointer to Map_info structure
  \param head_only TRUE to read only header
  
  \return 0 on success
  \return 1 file does not exist
  \return -1 on error
*/
int Vect_open_topo(struct Map_info *Map, int head_only)
{
    int err, ret;
    char buf[GPATH_MAX], file_path[GPATH_MAX];
    struct gvfile fp;
    struct Coor_info CInfo;
    struct Plus_head *Plus;

    G_debug(1, "Vect_open_topo(): name = %s mapset= %s", Map->name,
            Map->mapset);

    Plus = &(Map->plus);

    sprintf(buf, "%s/%s", GV_DIRECTORY, Map->name);
    G_file_name(file_path, buf, GV_TOPO_ELEMENT, Map->mapset);

    if (access(file_path, F_OK) != 0)   /* does not exist */
        return 1;

    dig_file_init(&fp);
    fp.file = G_fopen_old(buf, GV_TOPO_ELEMENT, Map->mapset);

    if (fp.file == NULL) {      /* topo file is not available */
        G_debug(1, "Cannot open topo file for vector '%s@%s'.",
                Map->name, Map->mapset);
        return -1;
    }

    /* get coor info */
    /* NOTE: coor file not yet opened */
    Vect_coor_info(Map, &CInfo);

    /* load head */
    if (dig_Rd_Plus_head(&fp, Plus) == -1)
        return -1;

    G_debug(1, "Topo head: coor size = %lu, coor mtime = %ld",
            (unsigned long)Plus->coor_size, Plus->coor_mtime);

    /* do checks */
    err = 0;
    if (CInfo.size != Plus->coor_size) {
        G_warning(_("Size of 'coor' file differs from value saved in topology file"));
        err = 1;
    }
    /* Do not check mtime because mtime is changed by copy */
    /*
       if ( CInfo.mtime != Plus->coor_mtime ) {
       G_warning ( "Time of last modification for 'coor' file differs from value saved in topo file.\n");
       err = 1;
       }
     */
    if (err) {
        G_warning(_("Please rebuild topology for vector map <%s@%s>"),
                  Map->name, Map->mapset);
        return -1;
    }

    /* load file to the memory */
    /* dig_file_load ( &fp); */

    /* load topo to memory */
    ret = dig_load_plus(Plus, &fp, head_only);

    fclose(fp.file);
    /* dig_file_free(&fp); */
    
    return ret == 0 ? -1 : 0;
}

/*!
  \brief Open spatial index file ('sidx')
  
  \param[in,out] Map pointer to Map_info
  \param mode 0 old, 1 update, 2 new
  
  \return 0 on success
  \return -1 on error
*/
int Vect_open_sidx(struct Map_info *Map, int mode)
{
    int err;
    struct Coor_info CInfo;
    struct Plus_head *Plus;

    G_debug(1, "Vect_open_sidx(): name = %s mapset= %s mode = %s", Map->name,
            Map->mapset, mode == 0 ? "old" : (mode == 1 ? "update" : "new"));

    Plus = &(Map->plus);

    if (Plus->Spidx_built) {
        G_debug(1, "Spatial index already opened");
        return 0;
    }

    dig_file_init(&(Plus->spidx_fp));

    if (mode < 2) {
        char *path, file_path[GPATH_MAX];
        
        path = Vect__get_path(Map);
        G_file_name(file_path, path, GV_SIDX_ELEMENT, Map->mapset);

        if (access(file_path, F_OK) != 0)       /* does not exist */
            return 1;

        Plus->spidx_fp.file = G_fopen_old(path, GV_SIDX_ELEMENT, Map->mapset);
        G_free(path);
        
        if (Plus->spidx_fp.file == NULL) {  /* sidx file is not available */
            G_debug(1, "Cannot open spatial index file for vector '%s@%s'.",
                    Map->name, Map->mapset);
            return -1;
        }

        /* get coor info */
        /* NOTE: coor file not yet opened */
        Vect_coor_info(Map, &CInfo);

        /* initialize spatial index */
        Plus->Spidx_new = FALSE;

	if (mode == 0) {
            /* free old indices */
            dig_spidx_free(Plus);
            /* initialize file based indices */
            Plus->Spidx_file = 1;
            dig_spidx_init(Plus);
	}

        /* load head */
        if (dig_Rd_spidx_head(&(Plus->spidx_fp), Plus) == -1) {
            fclose(Plus->spidx_fp.file);
            return -1;
        }

        G_debug(1, "Sidx head: coor size = %lu, coor mtime = %ld",
                (unsigned long)Plus->coor_size, Plus->coor_mtime);

        /* do checks */
        err = 0;
        if (CInfo.size != Plus->coor_size) {
            G_warning(_("Size of 'coor' file differs from value saved in sidx file"));
            err = 1;
        }
        /* Do not check mtime because mtime is changed by copy */
        /*
           if ( CInfo.mtime != Plus->coor_mtime ) {
           G_warning ( "Time of last modification for 'coor' file differs from value saved in topo file.\n");
           err = 1;
           }
         */
        if (err) {
            G_warning(_("Please rebuild topology for vector map <%s@%s>"),
                      Map->name, Map->mapset);
            fclose(Plus->spidx_fp.file);
            return -1;
        }
    }

    if (mode) {
        /* open new spatial index */
        Plus->Spidx_new = TRUE;
        
        /* file based or memory based */
        if (getenv("GRASS_VECTOR_LOWMEM")) {
            /* free old indices */
            dig_spidx_free(Plus);
            /* initialize file based indices */
            Plus->Spidx_file = 1;
            dig_spidx_init(Plus);
        }
        G_debug(1, "%s based spatial index",
                   Plus->Spidx_file == 0 ? "Memory" : "File");

        if (mode == 1) {
            /* load spatial index for update */
            if (dig_Rd_spidx(&(Plus->spidx_fp), Plus) == -1) {
                fclose(Plus->spidx_fp.file);
                return -1;
            }
        }
    }

    Plus->Spidx_built = TRUE;

    return 0;
}

/* check for external formats definition */
int map_format(struct Map_info *Map)
{
    int format;
    char *def_file;
    
    format = GV_FORMAT_NATIVE;
    /* temporary maps can be stored only in native format */
    if (Map->temporary)
        return format;
    
    if (G_find_file2("", "OGR", G_mapset())) {
        /* OGR */
        FILE *fp;
        const char *p;
        
        struct Key_Value *key_val;
        struct Format_info_ogr *ogr_info;
        
        G_debug(2, " using OGR format");
        if (getenv("GRASS_VECTOR_EXTERNAL_IMMEDIATE")) {
            /* vector features are written directly to OGR layer */
            format = GV_FORMAT_OGR;
        }
        else {
            /* vector features are written to the temporary vector map
             * in the native format and when closing the map
             * transfered to output OGR layer */
            format = GV_FORMAT_NATIVE;
            Map->temporary = TRUE;
        }
        fp = G_fopen_old("", "OGR", G_mapset());
        if (!fp) {
            G_fatal_error(_("Unable to open OGR file"));
        }
        key_val = G_fread_key_value(fp);
        fclose(fp);
        
        ogr_info = &(Map->fInfo.ogr);
        /* format */
        p = G_find_key_value("format", key_val);
        if (p)
            ogr_info->driver_name = G_store(p);
        /* dsn */
        p = G_find_key_value("dsn", key_val);
        if (p)
            ogr_info->dsn = G_store(p);
        /* options */
        p = G_find_key_value("options", key_val);
        if (p)
            ogr_info->layer_options = G_tokenize(p, ",");
        
        ogr_info->layer_name = G_store(Map->name);
    }
    
    def_file = getenv("GRASS_VECTOR_PGFILE");  /* GRASS_VECTOR_PGFILE defined by v.out.postgis */
    if (G_find_file2("", def_file ? def_file : "PG", G_mapset())) {
        /* PostGIS */
        if (Map->fInfo.ogr.driver_name) {
            G_warning(_("OGR output also detected, using OGR"));
        }
        else {
            FILE *fp;
            const char *p;
            
            struct Key_Value *key_val;
            struct Format_info_pg *pg_info;
            
            G_debug(2, " using PostGIS format");
            fp = G_fopen_old("", def_file ? def_file : "PG", G_mapset());
            if (!fp) {
                G_fatal_error(_("Unable to open PG file"));
            }
            key_val = G_fread_key_value(fp);
            fclose(fp);
            
            pg_info = &(Map->fInfo.pg);
            /* conninfo */
            p = G_find_key_value("conninfo", key_val);
            if (p) {
                pg_info->conninfo = G_store(p);
                G_debug(1, "PG: conninfo = '%s'", pg_info->conninfo);
            }
            
            /* schema (default: public) */
            p = G_find_key_value("schema", key_val);
            if (p)
                pg_info->schema_name = G_store(p);
            else
                pg_info->schema_name = G_store("public");
            G_debug(1, "PG: schema_name = '%s'", pg_info->schema_name);
            
            /* fid column (default: FID_COLUMN) */
            p = G_find_key_value("fid", key_val);
            if (p)
                pg_info->fid_column = G_store(p);
            else
#ifdef HAVE_POSTGRES
                pg_info->fid_column = G_store(GV_PG_FID_COLUMN);
#endif
            G_debug(1, "PG: fid_column = '%s'", pg_info->fid_column);
            
            /* geometry column (default: GEOMETRY_COLUMN) */
            p = G_find_key_value("geometry_name", key_val);
            if (p)
                pg_info->geom_column = G_store(p);
            else
#ifdef HAVE_POSTGRES
                pg_info->geom_column = G_store(GV_PG_GEOMETRY_COLUMN);
#endif
            G_debug(1, "PG: geom_column = '%s'", pg_info->geom_column);
            
            /* srid (default: 0) */
            p = G_find_key_value("srid", key_val);
            if (p)
                pg_info->srid = atoi(p);
            G_debug(1, "PG: srid = %d", pg_info->srid);
            
            /* table name */
            Map->fInfo.pg.table_name = G_store(Map->name);

            /* PostGIS topology enabled ? */
            p = G_find_key_value("topology", key_val);
            if (p && G_strcasecmp(p, "yes") == 0) {
                /* define topology name
                   this should be configurable by the user
                */
                G_asprintf(&(pg_info->toposchema_name), "topo_%s",
                           pg_info->table_name);
            }
            
            if (getenv("GRASS_VECTOR_EXTERNAL_IMMEDIATE")) {
                /* vector features are written directly to PostGIS layer */
                format = GV_FORMAT_POSTGIS;
            }
            else {
                /* vector features are written to the temporary vector map
                 * in the native format and when closing the map
                 * transfered to output PostGIS layer */
                format = GV_FORMAT_NATIVE;
                Map->temporary = TRUE;
            }
        }
    }
    
    G_debug(2, "map_format = %d", format);
    return format;
}

/*!
  \brief Get map directory name (internal use only)

  Allocate string should be freed by G_free().

  \param Map pointer to Map_info struct

  \return allocated buffer containing path
*/
char *Vect__get_path(const struct Map_info *Map)
{
    char path[GPATH_MAX];
    
    if (Map->temporary) {
        char path_tmp[GPATH_MAX];
        G__temp_element(path_tmp);
        sprintf(path, "%s/%s/%s", path_tmp, GV_DIRECTORY, Map->name);
    }
    else {
        sprintf(path, "%s/%s", GV_DIRECTORY, Map->name);
    }
    
    return G_store(path);
}

/*!
  \brief Get map element full path (internal use only)

  Allocate string should be freed by G_free().

  \param Map pointer to Map_info struct
  \param element element name, eg. GV_TOPO_ELEMENT

  \return allocated buffer containing path
*/
char *Vect__get_element_path(const struct Map_info *Map, const char *element)
{
    char file_path[GPATH_MAX], *path;
    
    path = Vect__get_path(Map);
    G_file_name(file_path, path, element, Map->mapset);
    G_free(path);

    return G_store(file_path);
}
