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
static int build_topogeom_stmt(const struct Format_info_pg *, int, int, char *);
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
    int i, type;
    char stmt[DB_SQL_MAX];
    
    struct Plus_head *plus;
    struct Format_info_pg *pg_info;
    
    plus = &(Map->plus);
    pg_info = &(Map->fInfo.pg);
    
    /* check if upgrade or downgrade */
    if (build < plus->built) { 
        /* -> downgrade */
        Vect__build_downgrade(Map, build);
        return 1;
    }
    
    if (build != GV_BUILD_ALL) {
        /* TODO: implement all build levels */
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

    /* update TopoGeometry based on GRASS-like topology */
    if (build < GV_BUILD_BASE)
        return 1; /* nothing to print */
    
    if (plus->built < GV_BUILD_BASE) {
        Vect_build_nat(Map, build);
    }
    
    i = 0;
    Vect_rewind(Map);
    if (Vect__execute_pg(pg_info->conn, "BEGIN"))
        return 0;
    
    G_message(_("Updating TopoGeometry data..."));
    while (TRUE) {
        type = Vect_read_next_line(Map, NULL, NULL);
        if (type == -1) {
            G_warning(_("Unable to read vector map"));
            return 0;
        }
        else if (type == -2) {
            break;
        }
        G_progress(++i, 1e3);
        
        /* update topogeometry elements in feature table */
        if (type == GV_POINT || type == GV_LINE) {
            if (build_topogeom_stmt(pg_info, i, type, stmt) &&
                Vect__execute_pg(pg_info->conn, stmt) == -1) {
                Vect__execute_pg(pg_info->conn, "ROLLBACK");
                return 0;
            }
        }
    }
    G_progress(1, 1);
    
    if (Vect__execute_pg(pg_info->conn, "COMMIT") == -1)
        return 0;

    return 1;
}

/*! 
  \brief Build UPDATE statement for topo geometry element stored in
  feature table

  \param pg_info so pointer to Format_info_pg
  \param type feature type (GV_POINT, ...)
  \param id topology element id
  \param[out] stmt string buffer
  
  \return 1 on success
  \return 0 on failure
*/
int build_topogeom_stmt(const struct Format_info_pg *pg_info,
                        int id, int type, char *stmt)
{
    int topogeom_type;
    
    if (type == GV_POINT)
        topogeom_type = 1;
    else if (type & GV_LINES)
        topogeom_type = 2;
    else {
        G_warning(_("Unsupported topo geometry type %d"), type);
        return 0;
    }
    
    /* it's quite slow...
       
    sprintf(stmt, "UPDATE \"%s\".\"%s\" SET %s = "
            "topology.CreateTopoGeom('%s', %d, 1,"
            "'{{%d, %d}}'::topology.topoelementarray) "
            "WHERE %s = %d",
            pg_info->schema_name, pg_info->table_name,
            pg_info->topogeom_column, pg_info->toposchema_name,
            topogeom_type, id, topogeom_type,
            pg_info->fid_column, id);
    */

    sprintf(stmt, "UPDATE \"%s\".\"%s\" SET %s = "
            "'(%d, 1, %d, %d)'::topology.TopoGeometry "
            "WHERE %s = %d",
            pg_info->schema_name, pg_info->table_name,
            pg_info->topogeom_column, pg_info->toposchema_id,
            id, topogeom_type, pg_info->fid_column, id);

    return 1;
}
#endif
