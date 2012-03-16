/*!
   \file lib/vector/Vlib/build_pg.c

   \brief Vector library - Building topology for PostGIS layers

   Higher level functions for reading/writing/manipulating vectors.

   \todo Implement build_topo()
   
   Line offset is
    - centroids   : FID
    - other types : index of the first record (which is FID) in offset array.
 
   (C) 2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_POSTGRES
#include <libpq-fe.h>

static int build_topo(struct Map_info *, int);
#endif

/*!
   \brief Build topology for PostGIS layer

   Build levels:
    - GV_BUILD_NONE
    - GV_BUILD_BASE
    - GV_BUILD_ATTACH_ISLES
    - GV_BUILD_CENTROIDS
    - GV_BUILD_ALL
   
   \param Map pointer to Map_info structure
   \param build build level

   \return 1 on success
   \return 0 on error
 */
int Vect_build_pg(struct Map_info *Map, int build)
{
#ifdef HAVE_POSTGRES
    struct Plus_head *plus;
    struct Format_info_pg *pg_info;

    plus     = &(Map->plus);
    pg_info  = &(Map->fInfo.pg);
    
    G_debug(1, "Vect_build_pg(): db='%s' table='%s', build=%d",
	    pg_info->db_name, pg_info->table_name, build);
    
    if (build == plus->built)
	return 1;		/* do nothing */
    
    /* TODO move this init to better place (Vect_open_ ?), because in
       theory build may be reused on level2 */
    if (build >= plus->built && build > GV_BUILD_BASE) {
	G_free((void *) pg_info->offset.array);
	G_zero(&(pg_info->offset), sizeof(struct Format_info_offset));
    }

    if (!pg_info->conn) {
	G_warning(_("No DB connection"));
	return 0;
    }

    if (!pg_info->fid_column) {
	G_warning(_("Feature table <%s> has no primary key defined"),
		  pg_info->table_name);
	G_warning(_("Random read is not supported by OGR for this layer. "
		    "Unable to build topology."));
	return 0;
    }
    
    if (build > GV_BUILD_NONE)
	G_message(_("Using external data format '%s' (feature type '%s')"),
		  Vect_get_finfo_format_info(Map),
		  Vect_get_finfo_geometry_type(Map));
    
    return Vect__build_sfa(Map, build);
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return 0;
#endif
}

#ifdef HAVE_POSTGRES
int build_topo(struct Map_info *Map, int build)
{
    G_fatal_error(_("Not implemented"));
    return 1;
}
#endif
