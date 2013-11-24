/*!
   \file lib/vector/Vlib/close_ogr.c

   \brief Vector library - Close map (OGR)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009, 2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and Piero Cavalieri.
*/

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include <ogr_api.h>
#endif

#include "local_proto.h"

/*!
  \brief Close vector map (OGR dsn & layer) on level 1

  \param Map pointer to Map_info structure

  \return 0 on success
  \return non-zero on error
*/
int V1_close_ogr(struct Map_info *Map)
{
#ifdef HAVE_OGR
    struct Format_info_ogr *ogr_info;
    
    G_debug(3, "V1_close_ogr() name = %s mapset = %s", Map->name, Map->mapset);
    
    if (!VECT_OPEN(Map))
	return -1;
    
    ogr_info = &(Map->fInfo.ogr);
    if (Map->format != GV_FORMAT_OGR_DIRECT &&
        (Map->mode == GV_MODE_WRITE || Map->mode == GV_MODE_RW)) {
        /* write header */
        Vect__write_head(Map);
        if (G_find_file2("", "OGR", G_mapset())) {
            /* write frmt file for created PG-link */
            Vect_save_frmt(Map);
        }
    }

    if (ogr_info->feature_cache)
	OGR_F_Destroy(ogr_info->feature_cache);

    /* destroy OGR datasource */
    OGR_DS_Destroy(ogr_info->ds);

    Vect__free_cache(&(ogr_info->cache));
    
    /* close DB connection (for atgtributes) */
    if (ogr_info->dbdriver) {
	db_close_database_shutdown_driver(ogr_info->dbdriver);
    }
    
    G_free(ogr_info->driver_name);
    G_free(ogr_info->dsn);
    G_free(ogr_info->layer_name);
    if (ogr_info->layer_options)
	G_free_tokens(ogr_info->layer_options);

    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}

/*!
  \brief Close vector map on topological level (write out fidx file)

  \param Map pointer to Map_info structure
  
  \return 0 on success
  \return non-zero on error
*/
int V2_close_ogr(struct Map_info *Map)
{
#ifdef HAVE_OGR
    struct Format_info_ogr *ogr_info;
  
    G_debug(3, "V2_close_ogr() name = %s mapset = %s", Map->name, Map->mapset);

    if (!VECT_OPEN(Map))
	return -1;

    ogr_info = &(Map->fInfo.ogr);
    
    /* write fidx for maps in the current mapset */
    if (Vect_save_fidx(Map, &(ogr_info->offset)) != 1)
	G_warning(_("Unable to save feature index file for vector map <%s>"),
		  Map->name);
    
    G_free(ogr_info->offset.array);
    
    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with OGR support"));
    return -1;
#endif
}
