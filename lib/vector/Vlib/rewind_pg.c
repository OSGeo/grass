/*!
   \file lib/vector/Vlib/rewind_pg.c

   \brief Vector library - rewind data (PostGIS layers)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2011-2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
 */

#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"
#endif

/*! 
   \brief Rewind vector map (PostGIS layer) to cause reads to start
   at beginning (level 1)

   \param Map pointer to Map_info structure

   \return 0 on success
   \return -1 on error
 */
int V1_rewind_pg(struct Map_info *Map)
{
    G_debug(2, "V1_rewind_pg(): name = %s", Map->name);

#ifdef HAVE_POSTGRES
    struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);

    /* reset reading */
    pg_info->next_line = 0;

    /* reset cache */
    pg_info->cache.lines_num = pg_info->cache.lines_next = 0;
    pg_info->cache.fid = -1;

    /* close DB cursor if necessary */
    if (pg_info->res) {
        char stmt[DB_SQL_MAX];

        PQclear(pg_info->res);
        pg_info->res = NULL;

        sprintf(stmt, "CLOSE %s_%s%p",
                pg_info->schema_name, pg_info->table_name, pg_info->conn);
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            G_warning(_("Unable to close cursor"));
            return -1;
        }
        Vect__execute_pg(pg_info->conn, "COMMIT");
    }

    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Rewind vector map (PostGIS layer) to cause reads to start
   at beginning on topological level (level 2)

   \param Map pointer to Map_info structure

   \return 0 on success
   \return -1 on error
 */
int V2_rewind_pg(struct Map_info *Map)
{
    G_debug(2, "V2_rewind_pg(): name = %s", Map->name);
#ifdef HAVE_POSTGRES
    Map->next_line = 1;

    V1_rewind_pg(Map);

    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}
