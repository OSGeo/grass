/*!
   \file lib/vector/Vlib/close_nat.c

   \brief Vector library - Close map (native format)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2015 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
*/

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

/*!
  \brief Close vector map

  \param Map vector map to be closed
  
  \return 0 on success
  \return non-zero on error
*/
int V1_close_nat(struct Map_info *Map)
{
    struct Coor_info CInfo;

    G_debug(1, "V1_close_nat(): name = %s mapset= %s", Map->name,
	    Map->mapset);
    if (!VECT_OPEN(Map))
	return 1;

    if (Map->mode == GV_MODE_WRITE || Map->mode == GV_MODE_RW) {
	Vect_coor_info(Map, &CInfo);
	Map->head.size = CInfo.size;
	dig__write_head(Map);

	Vect__write_head(Map);
	Vect_write_dblinks(Map);
    }

    /* close coor file */
    fclose(Map->dig_fp.file);
    dig_file_free(&(Map->dig_fp));

    /* delete temporary map ? */
    if (Map->temporary) {
        int delete;
        char *env = getenv("GRASS_VECTOR_TEMPORARY");

        delete = TRUE;
        if (Map->temporary == TEMPORARY_MAP_ENV && env) {
            if (G_strcasecmp(env, "move") == 0) {
                /* copy temporary vector map to the current mapset */
                char path_tmp[GPATH_MAX], path_map[GPATH_MAX];
                    
                G_debug(1, "V1_close_nat(): temporary map <%s> TO BE MOVED TO"
                        " CURRENT MAPSET",
                        Map->name);
                Vect__get_element_path(path_tmp, Map, NULL);

                G_file_name(path_map, GV_DIRECTORY, NULL, Map->mapset);
                if (access(path_map, 0) != 0 && G_mkdir(path_map) != 0)
                    G_fatal_error(_("Unable to create '%s': %s"),
                                  path_map, strerror(errno));

                G_file_name(path_map, GV_DIRECTORY, Map->name, Map->mapset);

                G_debug(1, "V1_close_nat(): %s -> %s", path_tmp, path_map);
                if (0 != G_recursive_copy(path_tmp, path_map))
                    G_fatal_error(_("Unable to copy '%s': %s"), path_tmp, strerror(errno));

#ifdef TEMPORARY_MAP_DB
                int i, ndblinks;
                int tmp;
                
                struct field_info *fi;
                dbConnection connection;
                struct dblinks *dblinks;

                G_debug(1, "V1_close_nat(): copying attributes");
                /* copy also attributes */
                dblinks = Vect_new_dblinks_struct();
                db_get_connection(&connection);
                ndblinks = Vect_get_num_dblinks(Map);
                for (i = 0; i < ndblinks; i++) {
                    fi = Vect_get_dblink(Map, i);
                    if (DB_OK != db_copy_table(fi->driver, fi->database, fi->table,
                                               connection.driverName,
                                               connection.databaseName,
                                               fi->table)) {
                        G_warning(_("Unable to copy table <%s>"), fi->table);
                        continue;
                    }

                    Vect_add_dblink(dblinks, fi->number, fi->name,
                                    fi->table, fi->key, connection.databaseName,
                                    connection.driverName);
                    G_free(fi);
                }
                G_free(Map->dblnk);
                Map->dblnk = dblinks;
                tmp = Map->temporary;
                Map->temporary = TEMPORARY_MAP_DISABLED;
                Vect_write_dblinks(Map);
                Map->temporary = tmp;
#endif
            }
            else if (G_strcasecmp(env, "delete") == 0) {
                /* delete temporary vector map */
                G_debug(1, "V1_close_nat(): temporary map <%s> TO BE DELETED", Map->name);
            }
            else {
                /* do not delete temporary vector map */
                G_debug(1, "V1_close_nat(): temporary map <%s> IS NOT DELETED", 
                        Map->name);
                delete = FALSE;
            }
        }
        else if (Map->temporary == TEMPORARY_MAP) {
            G_debug(1, "V1_close_nat(): temporary map <%s> TO BE DELETED", Map->name);
            delete = TRUE;
        }
        
        if (delete) {
            char path_tmp[GPATH_MAX];

            /* delete vector directory */
            Vect__get_element_path(path_tmp, Map, NULL);
            G_recursive_remove(path_tmp);

#ifndef TEMPORARY_MAP_DB
            if (G_strcasecmp(env, "move") != 0) {
                int i, ndblinks;

                dbDriver *driver;
                dbString table_name;
                
                struct field_info *fi;
                
                db_init_string(&table_name);
                
                /* drop also attribute table */
                ndblinks = Vect_get_num_dblinks(Map);
                for (i = 0; i < ndblinks; i++) {
                    fi = Vect_get_dblink(Map, i);
                    
                    driver = db_start_driver_open_database(fi->driver, fi->database);
                    if (driver == NULL) {
                        G_warning(_("Unable to open database <%s> by driver <%s>"),
                                  fi->database, fi->driver);
                        continue;
                    }
                    
                    db_set_string(&table_name, fi->table);
                    if (DB_OK != db_drop_table(driver, &table_name)) {
                        G_warning(_("Unable to drop table <%s>"), fi->table);
                        continue;
                    }
                }
            }
#endif
        }
    }

    return 0;
}
