/*!
   \file lib/vector/Vlib/close.c

   \brief Vector library - Close vector map

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009, 2011-2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
   \author Update to GRASS 7 Martin Landa <landa.martin gmail.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

static int clo_dummy()
{
    return -1;
}

#if !defined HAVE_OGR || !defined HAVE_POSTGRES
static int format()
{
    G_fatal_error(_("Requested format is not compiled in this version"));
    return 0;
}
#endif

static int (*Close_array[][2]) () = {
    {
    clo_dummy, V1_close_nat}
#ifdef HAVE_OGR
    , {
    clo_dummy, V1_close_ogr}
    , {
    clo_dummy, V1_close_ogr}
#else
    , {
    clo_dummy, format}
    , {
    clo_dummy, format}
#endif
#ifdef HAVE_POSTGRES
    , {
    clo_dummy, V1_close_pg}
#else
    , {
    clo_dummy, format}
#endif
};

static void unlink_file(const struct Map_info *, const char *);

/*!
   \brief Close vector map

   \param Map pointer to Map_info

   \return 0 on success
   \return non-zero on error
 */
int Vect_close(struct Map_info *Map)
{
    int create_link; /* used for external formats only */
    struct Coor_info CInfo;
    
    G_debug(1, "Vect_close(): name = %s, mapset = %s, format = %d, level = %d, is_tmp = %d",
	    Map->name, Map->mapset, Map->format, Map->level, Map->temporary);

    if (Map->temporary &&
        (Map->fInfo.ogr.dsn || Map->fInfo.pg.conninfo)) {
        /* transfer features for external output format */
        struct Map_info Out;
        
        putenv("GRASS_VECTOR_EXTERNAL_IMMEDIATE=1");
        if (-1 == Vect_open_new(&Out, Vect_get_name(Map), Vect_is_3d(Map))) {
            G_warning(_("Unable to create vector map <%s>"),
                      Vect_get_name(Map));
            return 1;
        }

        /* copy metadata */
        Vect_hist_copy(Map, &Out);
        Vect_copy_head_data(Map, &Out);
        /* copy dblinks (temporary map -> output map) to transfer
           (input map -> output map) attributes */
        Vect_copy_map_dblinks(Map, &Out, TRUE);
        /* afterwords, dblinks must be removed from temporary map
           otherwise when deleting temporary map also original
           attribute tables would be deteled */
        Vect_map_del_dblink(Map, -1); /* delete db links for all layers */
        
        if (0 != Vect_copy_map_lines_field(Map, 1, &Out)) { /* always layer = 1 for OGR/PG maps */
            G_warning(_("Copying features failed"));
            return -1;
        }

        Vect_build(&Out);
        
        Vect_close(&Out);
        putenv("GRASS_VECTOR_EXTERNAL_IMMEDIATE="); /* unset variable */
    }
    
    /* check for external formats whether to create a link */
    create_link = TRUE;
    if (Map->format == GV_FORMAT_OGR ||
        Map->format == GV_FORMAT_POSTGIS) {
        char *def_file;
        
        if (Map->format == GV_FORMAT_POSTGIS) {
            if (getenv("GRASS_VECTOR_PGFILE"))
                def_file = getenv("GRASS_VECTOR_PGFILE");
            else
                def_file = "PG";
        }
        else {
            def_file = "OGR";
        }
        if (G_find_file2("", def_file, G_mapset())) {
            FILE       *fp;
            const char *p;
            
            struct Key_Value *key_val;
            
            fp = G_fopen_old("", def_file, G_mapset());
            if (!fp) {
                G_warning(_("Unable to open %s file"), def_file);
            }
            else {
                key_val = G_fread_key_value(fp);
                fclose(fp);
                
                /* create a vector link in the current mapset ? */
                p = G_find_key_value("link", key_val);
                if (p && G_strcasecmp(p, "no") == 0) {
                    create_link = FALSE;
                }
                else {
                    p = G_find_key_value("link_name", key_val);
                    if (p) {
                        /* use different name for a link */
                        G_free(Map->name);
                        Map->name = G_store(p);
                    }
                }
            }
        }
    }
    
    /* store support files for vector maps in the current mapset if in
       write mode on level 2 */
    if (strcmp(Map->mapset, G_mapset()) == 0 &&
        Map->support_updated &&
        Map->plus.built == GV_BUILD_ALL &&
        create_link) {

        unlink_file(Map, GV_TOPO_ELEMENT); /* topo */

	unlink_file(Map, GV_SIDX_ELEMENT); /* sidx */

	unlink_file(Map, GV_CIDX_ELEMENT); /* cidx */

	if (Map->format == GV_FORMAT_OGR || Map->format == GV_FORMAT_POSTGIS) {
	    unlink_file(Map, GV_FIDX_ELEMENT); /* fidx */
	}
	
	Vect_coor_info(Map, &CInfo);
	Map->plus.coor_size = CInfo.size;
	Map->plus.coor_mtime = CInfo.mtime;

	/* write out topo file */
        Vect_save_topo(Map);
        
	/* write out sidx file */
	Map->plus.Spidx_new = TRUE; /* force writing */
	Vect_save_sidx(Map);

	/* write out cidx file */
	Vect_cidx_save(Map);
	
	/* write out fidx file */
	if (Map->format == GV_FORMAT_OGR)
	    V2_close_ogr(Map);
	else if (Map->format == GV_FORMAT_POSTGIS)
            V2_close_pg(Map);
    }
    /* spatial index must also be closed when opened with topo but
     * not modified */
    /* NOTE: also close sidx for GV_FORMAT_OGR if not direct OGR access */
    if (Map->format != GV_FORMAT_OGR_DIRECT &&
	Map->plus.Spidx_built == TRUE &&
	!Map->support_updated &&
	Map->plus.built == GV_BUILD_ALL &&
	create_link) {

	fclose(Map->plus.spidx_fp.file);
    }
    if (Map->level > 1 && Map->plus.release_support) {
	G_debug(1, "free topology, spatial index, and category index");
	dig_free_plus(&(Map->plus));
    }

    G_debug(1, "close history file");
    if (Map->hist_fp)
        fclose(Map->hist_fp);
    
    /* close level 1 files / data sources if not head_only */
    if (!Map->head_only) {
	if (create_link && ((*Close_array[Map->format][1]) (Map)) != 0) {
	    G_warning(_("Unable to close vector <%s>"),
		      Vect_get_full_name(Map));
	    return 1;
	}
    }

    G_free(Map->name);
    G_free(Map->mapset);
    G_free(Map->location);
    G_free(Map->gisdbase);
    
    Map->open = VECT_CLOSED_CODE;

    return 0;
}

/*!
   \brief Save format definition file for vector map

   \param Map pointer to Map_info structure

   \return 1 on success
   \return 0 on error
 */
int Vect_save_frmt(struct Map_info *Map)
{
    FILE *fd;
    char buf[GPATH_MAX];
    
    if (Map->format != GV_FORMAT_OGR &&
        Map->format != GV_FORMAT_POSTGIS) {
        G_warning(_("Invalid request for writing frmt file - map format is %d"), Map->format);
        return 0;
    }
    
    /* create frmt file */
    sprintf(buf, "%s/%s", GV_DIRECTORY, Map->name);
    fd = G_fopen_new(buf, GV_FRMT_ELEMENT);
    if (fd == NULL) {
        G_fatal_error("Unable to create file '%s'", buf);
    }

    if (Map->format == GV_FORMAT_POSTGIS) {
#ifdef HAVE_POSTGRES
        fprintf(fd, "format: postgis\n");
        fprintf(fd, "conninfo: %s\n", Map->fInfo.pg.conninfo);
        fprintf(fd, "schema: %s\n",   Map->fInfo.pg.schema_name);
        fprintf(fd, "table: %s\n",    Map->fInfo.pg.table_name);
#else
        G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
        return 0;
#endif
    } else if (Map->format == GV_FORMAT_OGR) {
#ifdef HAVE_OGR
        fprintf(fd, "format: ogr\n");
        fprintf(fd, "dsn: %s\n",   Map->fInfo.ogr.dsn);
        fprintf(fd, "layer: %s\n", Map->fInfo.ogr.layer_name);
#else
        G_fatal_error(_("GRASS is not compiled with OGR support"));
        return 0;
#endif
    }

    G_verbose_message(_("Link to vector map <%s> created"), Map->name);

    /* close frmt file */
    fclose(fd);

    return 1;
}

/*! Free memory of line cache

  \param cache pointer to lines cache to be freed
*/
void Vect__free_cache(struct Format_info_cache *cache) {
    int i;
    /* destroy lines in cache */
    for (i = 0; i < cache->lines_alloc; i++) {
	Vect_destroy_line_struct(cache->lines[i]);
    }
    G_free(cache->lines);
    G_free(cache->lines_types);
    G_free(cache->lines_cats);

    G_zero(cache, sizeof(struct Format_info_cache));
}

/*! Free memory of offset array

  \param cache pointer to offset array to be freed
*/
void Vect__free_offset(struct Format_info_offset *offset)
{
    G_free(offset->array);
    G_zero(offset, sizeof(struct Format_info_offset));
}   

void unlink_file(const struct Map_info *Map, const char *name)
{
    char *path;

    /* delete old support files if available */
    path = Vect__get_element_path(Map, name);
    if (access(path, F_OK) == 0) { /* file exists? */
        G_debug(2, "\t%s: unlink", path);
        unlink(path);
    }

    G_free(path);
}
