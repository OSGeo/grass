/*!
   \file lib/vector/Vlib/build_pg.c

   \brief Vector library - Building topology for PostGIS layers

   Higher level functions for reading/writing/manipulating vectors.

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
#include "pg_local_proto.h"

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

    plus = &(Map->plus);
    pg_info = &(Map->fInfo.pg);

    G_debug(1, "Vect_build_pg(): db='%s' table='%s', build=%d",
	    pg_info->db_name, pg_info->table_name, build);

    if (build == plus->built)
	return 1;		/* do nothing */

    /* TODO move this init to better place (Vect_open_ ?), because in
       theory build may be reused on level2 */
    if (build >= plus->built && build > GV_BUILD_BASE) {
	G_free((void *)pg_info->offset.array);
	G_zero(&(pg_info->offset), sizeof(struct Format_info_offset));
    }

    if (!pg_info->conn) {
	G_warning(_("No DB connection"));
	return 0;
    }

    if (!pg_info->fid_column) {
	G_warning(_("Feature table <%s> has no primary key defined"),
		  pg_info->table_name);
	G_warning(_("Random read is not supported for this layer. "
		    "Unable to build topology."));
	return 0;
    }

    /* commit transaction block (update mode only) */
    if (pg_info->inTransaction && Vect__execute_pg(pg_info->conn, "COMMIT") == -1)
	return 0;

    pg_info->inTransaction = FALSE;

    if (build > GV_BUILD_NONE) {
	G_message(_("Using external data format '%s' (feature type '%s')"),
		  Vect_get_finfo_format_info(Map),
		  Vect_get_finfo_geometry_type(Map)); 
        if (!pg_info->toposchema_name)
            G_message(_("Building pseudo-topology over simple features..."));
        else
            G_message(_("Building topology from PostGIS topology schema <%s>..."),
                      pg_info->toposchema_name);
    }

    if (!pg_info->toposchema_name)
        return Vect__build_sfa(Map, build);
    
    return build_topo(Map, build);
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return 0;
#endif
}

#ifdef HAVE_POSTGRES
/*!
  \brief Build from PostGIS topology schema

  Currently only GV_BUILD_ALL is supported

  \param Map pointer to Map_info struct
  \param build build level

  \return 1 on success
  \return 0 on error
*/
int build_topo(struct Map_info *Map, int build)
{
    struct Plus_head *plus;
    
    plus = &(Map->plus);
    
    /* check if upgrade or downgrade */
    if (build < plus->built) { 
        /* -> downgrade */
        Vect__build_downgrade(Map, build);
        return 1;
    }
    
    if (build != GV_BUILD_ALL) {
        G_warning(_("Only %s is supported for PostGIS topology"),
                  "GV_BUILD_ALL");
        return 0;
    }
    
    /* read topology from PostGIS ???
    if (plus->built < GV_BUILD_BASE) {
        if (load_plus(Map, FALSE) != 0)
            return 0;
    }
    */

    /* build GRASS-like topology from PostGIS topological
       primitives */
    if (Vect_build_nat(Map, build) != 1)
        return 0;
    
    /* update PostGIS topology based on GRASS-like topology */
    return 1;
}
#endif
