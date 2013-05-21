/*!
   \file lib/vector/Vlib/area_pg.c

   \brief Vector library - area-related functions (PostGIS Topology)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2013 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/vector.h>

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"

static PGresult *build_stmt(const struct Plus_head *, const struct Format_info_pg *, const plus_t *, int);

/*!
  \brief Get area boundary points (PostGIS Topology)
  
  Used by Vect_build_line_area() and Vect_get_area_points().

  \param Map pointer to Map_info struct
  \param lines array of boundary lines
  \param n_lines number of lines in array
  \param[out] APoints pointer to output line_pnts struct

  \return number of points
  \return -1 on error
*/
int Vect__get_area_points_pg(const struct Map_info *Map, const plus_t *lines, int n_lines,
                             struct line_pnts *APoints)
{
    int i, direction;
    
    struct Format_info_pg *pg_info;
    
    PGresult *res;
    
    pg_info = (struct Format_info_pg *)&(Map->fInfo.pg);
   
    Vect_reset_line(APoints);

    res = build_stmt(&(Map->plus), pg_info, lines, n_lines);
    if (!res)
        return -1;
    
    for (i = 0; i < n_lines; i++) {
        Vect__cache_feature_pg(PQgetvalue(res, i, 0), FALSE, FALSE,
                               &(pg_info->cache), NULL); /* do caching in readable way */
	direction = lines[i] > 0 ? GV_FORWARD : GV_BACKWARD;
	Vect_append_points(APoints, pg_info->cache.lines[0], direction);
	APoints->n_points--;	/* skip last point, avoids duplicates */
    }
    APoints->n_points++;	/* close polygon */

    PQclear(res);
    
    return APoints->n_points;
}

PGresult *build_stmt(const struct Plus_head *plus, const struct Format_info_pg *pg_info,
                     const plus_t *lines, int n_lines)
{
    int i, line;
    size_t stmt_id_size;
    char *stmt, *stmt_id, buf_id[128];

    struct P_line *BLine;
    
    PGresult *res;
    
    stmt = NULL;
    stmt_id_size = DB_SQL_MAX;
    stmt_id = (char *) G_malloc(stmt_id_size);
    stmt_id[0] = '\0';

    for (i = 0; i < n_lines; i++) {
        if (strlen(stmt_id) + 100 > stmt_id_size) {
            stmt_id_size = strlen(stmt_id) + DB_SQL_MAX;
            stmt_id = (char *) G_realloc(stmt_id, stmt_id_size);
        }
	line = abs(lines[i]);
        BLine = plus->Line[line];
        if (i > 0)
            strcat(stmt_id, ",");
        sprintf(buf_id, "%d", (int) BLine->offset);
        strcat(stmt_id, buf_id);
    }
    G_asprintf(&stmt, "SELECT geom FROM \"%s\".edge_data WHERE edge_id IN (%s) "
               "ORDER BY POSITION(edge_id::text in '%s')", pg_info->toposchema_name,
               stmt_id, stmt_id);
    G_free(stmt_id);
    
    G_debug(2, "SQL: %s", stmt);
    res = PQexec(pg_info->conn, stmt);
    G_free(stmt);

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
        PQntuples(res) != n_lines) {
        if (res)
            PQclear(res);
        
        return NULL;
    }

    return res;
}
#endif
