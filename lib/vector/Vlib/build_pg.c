/*!
   \file lib/vector/Vlib/build_pg.c

   \brief Vector library - Building topology for PostGIS layers

   Higher level functions for reading/writing/manipulating vectors.

   Line offset (simple features only) is
   - centroids   : FID
   - other types : index of the first record (which is FID) in offset array.

   (C) 2012-2013 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
 */

#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"

static int build_topo(struct Map_info *, int);
static int build_topogeom_stmt(const struct Format_info_pg *, int, int, int, char *);
static int save_map_bbox(const struct Format_info_pg *, const struct bound_box*);
static int create_topo_grass(const struct Format_info_pg *);
static int has_topo_grass(const struct Format_info_pg *);
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

    /* commit transaction block (update mode only) */
    if (pg_info->inTransaction && Vect__execute_pg(pg_info->conn, "COMMIT") == -1)
        return 0;
    pg_info->inTransaction = FALSE;
    
    if (pg_info->feature_type == SF_UNKNOWN)
        return 1;
    
    if (build == plus->built)
        return 1;            /* do nothing */

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

    if (!pg_info->fid_column && !pg_info->toposchema_name) {
        G_warning(_("Feature table <%s> has no primary key defined"),
                  pg_info->table_name);
        G_warning(_("Random read is not supported for this layer. "
                    "Unable to build topology."));
        return 0;
    }

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

    if (!pg_info->toposchema_name) /* pseudo-topology for simple features */
        return Vect__build_sfa(Map, build);
    
    /* PostGIS Topology */
    return build_topo(Map, build);
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return 0;
#endif
}

#ifdef HAVE_POSTGRES
/*!
  \brief Build from PostGIS topology schema

  \todo Attach isles

  \param Map pointer to Map_info struct
  \param build build level

  \return 1 on success
  \return 0 on error
*/
int build_topo(struct Map_info *Map, int build)
{
    int line, type, s;
    int area, nareas, isle, nisles;
    int face[2];
    char stmt[DB_SQL_MAX];
    
    struct Plus_head *plus;
    struct Format_info_pg *pg_info;

    struct P_line *Line;
    struct P_area *Area;
    struct P_topo_b *topo_b;
    struct P_isle *Isle;
    
    plus = &(Map->plus);
    pg_info = &(Map->fInfo.pg);
    
    /* check if upgrade or downgrade */
    if (build < plus->built) { 
        /* -> downgrade */
        Vect__build_downgrade(Map, build);
        return 1;
    }
    /*     -> upgrade */
    
    if (build < GV_BUILD_BASE)
        return 1; /* nothing to print */
    
    /* update TopoGeometry based on GRASS-like topology */
    Vect_build_nat(Map, build);
    
    /* store map boundig box in DB */
    save_map_bbox(pg_info, &(plus->box));
    
    /* begin transaction */
    if (Vect__execute_pg(pg_info->conn, "BEGIN"))
        return 0;
    
    /* update faces from GRASS Topology */
    if (build >= GV_BUILD_AREAS) {
        /* do clean up (1-3)
           insert new faces (4)
           update edges (5)
        */
        /* 1) reset centroids to '0' (universal face) */
        sprintf(stmt, "UPDATE \"%s\".node SET containing_face = 0 WHERE "
                "containing_face IS NOT NULL", pg_info->toposchema_name);
        G_debug(2, "SQL: %s", stmt);
        if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return 0;
        }

        /* 2) reset left|right edges */
        sprintf(stmt, "UPDATE \"%s\".edge_data SET left_face = 0, right_face = 0",
                pg_info->toposchema_name);
        G_debug(2, "SQL: %s", stmt);
        if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return 0;
        }

        /* 3) delete faces */        
        sprintf(stmt, "DELETE FROM \"%s\".face WHERE "
                "face_id != 0", pg_info->toposchema_name);
        G_debug(2, "SQL: %s", stmt);
        if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return 0;
        }
        
        /* 4) insert faces & update nodes (containing_face) based on
         * GRASS topology */
        G_message(_("Updating faces..."));
        nareas = Vect_get_num_areas(Map);
        for (area = 1; area <= nareas; area++) {
            G_percent(area, nareas, 5);
            if (0 == Vect__insert_face_pg(Map, area)) {
                Vect__execute_pg(pg_info->conn, "ROLLBACK");
                return 0;
            }
            
            if (build < GV_BUILD_CENTROIDS)
                continue;
            
            /* update centroids (node -> containing_face) */
            Area = plus->Area[area];
            if (Area->centroid < 1) {
                G_debug(3, "Area %d without centroid, skipped", area);
                continue;
            }
            
            Line = plus->Line[Area->centroid];
            sprintf(stmt, "UPDATE \"%s\".node SET "
                    "containing_face = %d WHERE node_id = %d",
                    pg_info->toposchema_name, area, (int)Line->offset);
            G_debug(2, "SQL: %s", stmt);
            
            if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
                Vect__execute_pg(pg_info->conn, "ROLLBACK");
                return 0;
            }
        }

        /* 5) update edges (left and right face) */ 
        G_message(_("Updating edges..."));
        for (line = 1; line <= plus->n_lines; line++) {
            G_percent(line, plus->n_lines, 5);
            type = Vect_read_line(Map, NULL, NULL, line); 
            if (type != GV_BOUNDARY)
                continue;
            
            Line = Map->plus.Line[line];
            if (!Line) {
                G_warning(_("Inconsistency in topology detected. "
                            "Dead line found."));
                return 0;
            }
            
            topo_b = (struct P_topo_b *) Line->topo;
            
            for (s = 0; s < 2; s++) { /* for both sides */
                face[s] = s == 0 ? topo_b->left : topo_b->right;
                if (face[s] < 0) {
                    /* isle */
                    Isle = plus->Isle[abs(face[s])];
                    face[s] = Isle->area;
                }
            }
            G_debug(3, "update edge %d: left_face = %d, right_face = %d",
                    (int)Line->offset, face[0], face[1]);
            
            sprintf(stmt, "UPDATE \"%s\".edge_data SET "
                    "left_face = %d, right_face = %d "
                    "WHERE edge_id = %d", pg_info->toposchema_name,
                    face[0], face[1], (int) Line->offset);
            
            if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
                Vect__execute_pg(pg_info->conn, "ROLLBACK");
                return 0;
            }
        }
    } /* build >= GV_BUILD_AREAS */
    
    if (build >= GV_BUILD_ATTACH_ISLES) {
        /* insert isles as faces with negative face_id */
        nisles = Vect_get_num_islands(Map);
        for(isle = 1; isle <= nisles; isle++) {
            Isle = plus->Isle[isle];
            Vect__insert_face_pg(Map, -isle);
        }
    } /* build >= GV_BUILD_ISLES */

    if (pg_info->feature_type == SF_POLYGON) {
        int centroid;
        
        G_message(_("Updating TopoGeometry data..."));
        for (area = 1; area <= plus->n_areas; area++) {
            G_percent(area, plus->n_areas, 5);
            centroid = Vect_get_area_centroid(Map, area);
            if (centroid < 1)
                continue;
        
            /* update topogeometry object: centroid -> face */
            if (build_topogeom_stmt(pg_info, GV_CENTROID, area, centroid, stmt) &&
                Vect__execute_pg(pg_info->conn, stmt) == -1) {
                Vect__execute_pg(pg_info->conn, "ROLLBACK");
                return 0;
            }
        }
    }

    if (Vect__execute_pg(pg_info->conn, "COMMIT") == -1)
        return 0;

    return 1;
}

/*! 
  \brief Build UPDATE statement for topo geometry element stored in
  feature table

  \param pg_info so pointer to Format_info_pg
  \param type feature type (GV_POINT, ...)
  \param topo_id topology element id
  \param fid feature id
  \param[out] stmt string buffer
  
  \return 1 on success
  \return 0 on failure
*/
int build_topogeom_stmt(const struct Format_info_pg *pg_info,
                        int type, int topo_id, int fid, char *stmt)
{
    int topogeom_type;
    
    switch(type) {
    case GV_POINT:
        topogeom_type = 1;
        break;
    case GV_LINE:
    case GV_BOUNDARY:
        topogeom_type = 2;
        break;
    case GV_CENTROID:
        topogeom_type = 3;
        break;
    default:
        G_warning(_("Unsupported topo geometry type %d"), type);
        return 0;
    }
    
    sprintf(stmt, "UPDATE \"%s\".\"%s\" SET %s = "
            "'(%d, 1, %d, %d)'::topology.TopoGeometry "
            "WHERE (%s).id = %d",
            pg_info->schema_name, pg_info->table_name,
            pg_info->topogeom_column, pg_info->toposchema_id,
            topo_id, topogeom_type, pg_info->topogeom_column, fid);

    return 1;
}

/*!
  \brief Store map bounding box in DB head table

  \param pg_info pointer to Format_info_pg struct
  \param box pointer to bounding box

  \return 1 on success
  \return 0 on failure
*/
int save_map_bbox(const struct Format_info_pg *pg_info, const struct bound_box *box)
{
    char stmt[DB_SQL_MAX];
    
    /* create if not exists */
    if (create_topo_grass(pg_info) == -1) {
	G_warning(_("Unable to create <%s.%s>"), TOPO_SCHEMA, TOPO_TABLE);
	return 0;
    }
    
    /* update bbox */
    if (has_topo_grass(pg_info)) {
	/* -> update */
	sprintf(stmt, "UPDATE \"%s\".\"%s\" SET %s = "
		"'BOX3D(%.12f %.12f %.12f, %.12f %.12f %.12f)'::box3d WHERE %s = %d",
		TOPO_SCHEMA, TOPO_TABLE, TOPO_BBOX,
		box->W, box->S, box->B, box->E, box->N, box->T,
		TOPO_ID, pg_info->toposchema_id);
    }
    else {
	/* -> insert */
	sprintf(stmt, "INSERT INTO \"%s\".\"%s\" (%s, %s) "
		"VALUES(%d, 'BOX3D(%.12f %.12f %.12f, %.12f %.12f %.12f)'::box3d)",
		TOPO_SCHEMA, TOPO_TABLE, TOPO_ID, TOPO_BBOX, pg_info->toposchema_id,
		box->W, box->S, box->B, box->E, box->N, box->T);
    }
    
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
	return -1;
    }
    
    return 1;
}

/*!
  \brief Creates 'topology.grass' table if not exists

  \return 0 table already exists
  \return 1 table successfully added
  \return -1 on error
*/
int create_topo_grass(const struct Format_info_pg *pg_info)
{
    char stmt[DB_SQL_MAX];

    PGresult *result;
    
    /* check if table exists */
    sprintf(stmt, "SELECT COUNT(*) FROM information_schema.tables "
	    "WHERE table_schema = '%s' AND table_name = '%s'",
	    TOPO_SCHEMA, TOPO_TABLE);
    result = PQexec(pg_info->conn, stmt);
    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        PQclear(result);
        return -1;
    }
    
    if (atoi(PQgetvalue(result, 0, 0)) == 1) {
	/* table already exists */
	PQclear(result);
	return 1;
    }
    PQclear(result);
    
    G_debug(1, "<%s.%s> created", TOPO_SCHEMA, TOPO_TABLE);
    
    /* create table */
    sprintf(stmt, "CREATE TABLE \"%s\".\"%s\" (%s INTEGER, %s box3d)",
	    TOPO_SCHEMA, TOPO_TABLE, TOPO_ID, TOPO_BBOX);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
	return -1;
    }
    /* add primary key */
    sprintf(stmt, "ALTER TABLE \"%s\".\"%s\" ADD PRIMARY KEY (%s)",
	    TOPO_SCHEMA, TOPO_TABLE, TOPO_ID);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
	return -1;
    }

    /* add constraint */
    sprintf(stmt, "ALTER TABLE \"%s\".\"%s\" ADD CONSTRAINT \"%s_%s_fkey\" "
	    "FOREIGN KEY (%s) REFERENCES topology.topology(id) ON DELETE CASCADE",
	    TOPO_SCHEMA, TOPO_TABLE, TOPO_TABLE, TOPO_ID, TOPO_ID);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
	return -1;
    }
    
    return 1;
}

/*!
  \brief Check if 'topology_id' exists in 'topology.grass'

  \param pg_info pointer to Format_info_pg struct

  \return TRUE if exists
  \return FALSE otherwise
  \return -1 on error
*/
int has_topo_grass(const struct Format_info_pg *pg_info)
{
    int has_topo;
    char stmt[DB_SQL_MAX];
    
    PGresult *result;
    
    sprintf(stmt, "SELECT COUNT(*) FROM \"%s\".\"%s\" "
	    "WHERE %s = %d",
	    TOPO_SCHEMA, TOPO_TABLE, TOPO_ID, pg_info->toposchema_id);
    result = PQexec(pg_info->conn, stmt);
    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        PQclear(result);
        return -1;
    }
    
    has_topo = FALSE;
    if (atoi(PQgetvalue(result, 0, 0)) == 1) {
	/* table already exists */
	has_topo = TRUE;
    }
    PQclear(result);

    return has_topo;
}
#endif
