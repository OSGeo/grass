/*!
   \file lib/vector/Vlib/close_pg.c

   \brief Vector library - Close map (PostGIS)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
 */

#include <stdlib.h>
#include <unistd.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"
#endif

/*!
   \brief Close vector map (PostGIS layer) on level 1

   \param Map pointer to Map_info structure

   \return 0 on success
   \return non-zero on error
 */
int V1_close_pg(struct Map_info *Map)
{
#ifdef HAVE_POSTGRES
    struct Format_info_pg *pg_info;

    G_debug(3, "V2_close_pg() name = %s mapset = %s", Map->name, Map->mapset);

    if (!VECT_OPEN(Map))
        return -1;

    pg_info = &(Map->fInfo.pg);
    if (Map->mode == GV_MODE_WRITE || Map->mode == GV_MODE_RW) {
        /* write header */
        Vect__write_head(Map);
        /* write frmt file for created PG-link */
        Vect_save_frmt(Map);
    }

    /* clear result */
    if (pg_info->res) {
        PQclear(pg_info->res);
        pg_info->res = NULL;
    }

    /* close open cursor */
    if (pg_info->cursor_name) {
        char stmt[DB_SQL_MAX];
        
        sprintf(stmt, "CLOSE %s", pg_info->cursor_name);
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            G_warning(_("Unable to close cursor %s"), pg_info->cursor_name);
            return -1;
        }
        Vect__execute_pg(pg_info->conn, "COMMIT");
        G_free(pg_info->cursor_name);
        pg_info->cursor_name = NULL;
    }

    PQfinish(pg_info->conn);

    /* close DB connection (for atgtributes) */
    if (pg_info->dbdriver) {
        db_close_database_shutdown_driver(pg_info->dbdriver);
    }

    Vect__free_cache(&(pg_info->cache));
    
    G_free(pg_info->db_name);
    G_free(pg_info->schema_name);
    G_free(pg_info->geom_column);
    G_free(pg_info->fid_column);

    if (pg_info->toposchema_name)
        G_free(pg_info->toposchema_name);

    if (pg_info->topogeom_column)
        G_free(pg_info->topogeom_column);

    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Close vector map (PostGIS layer) on topological level (write out fidx file)

   \param Map pointer to Map_info structure

   \return 0 on success
   \return non-zero on error
 */
int V2_close_pg(struct Map_info *Map)
{
#ifdef HAVE_POSTGRES
    G_debug(3, "V2_close_pg() name = %s mapset = %s", Map->name, Map->mapset);

    if (!VECT_OPEN(Map))
        return -1;

    if (Map->fInfo.pg.toposchema_name) {
        /* no fidx file for PostGIS topology
           
           remove topo file (which was required for saving sidx file)
        */
        char buf[GPATH_MAX];
        char file_path[GPATH_MAX];
        
        /* delete old support files if available */
        sprintf(buf, "%s/%s", GV_DIRECTORY, Map->name);
        
        G_file_name(file_path, buf, GV_TOPO_ELEMENT, G_mapset());
        if (access(file_path, F_OK) == 0) /* file exists? */
            unlink(file_path);
        
        return 0;
    }
    
    /* write fidx for maps in the current mapset */
    if (Vect_save_fidx(Map, &(Map->fInfo.pg.offset)) != 1)
        G_warning(_("Unable to save feature index file for vector map <%s>"),
                  Map->name);

    Vect__free_offset(&(Map->fInfo.pg.offset));

    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}
