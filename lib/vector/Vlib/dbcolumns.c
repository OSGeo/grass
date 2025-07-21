/*!
   \file lib/vector/Vlib/dbcolumns.c

   \brief Vector library - DB info on vectors maps

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2005-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Markus Neteler
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grass/glocale.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

#define BUFF_MAX 2000

/*!
   \brief Fetches list of DB column names of vector map attribute table

   \param Map vector map
   \param field layer number

   \return list of column(s) names on success
   \return NULL on error
 */
const char *Vect_get_column_names(struct Map_info *Map, int field)
{
    int num_dblinks, ncols, col;
    struct field_info *fi = NULL;
    dbDriver *driver = NULL;
    dbHandle handle;
    dbString table_name;
    dbTable *table;
    const char **col_names;
    char *list = NULL;

    num_dblinks = Vect_get_num_dblinks(Map);
    if (num_dblinks <= 0)
        return (NULL);

    G_debug(3, "Displaying column names for database connection of layer %d:",
            field);
    if ((fi = Vect_get_field(Map, field)) == NULL)
        return (NULL);
    driver = db_start_driver(fi->driver);
    if (driver == NULL) {
        Vect_destroy_field_info(fi);
        return (NULL);
    }
    db_init_handle(&handle);
    db_set_handle(&handle, fi->database, NULL);
    if (db_open_database(driver, &handle) != DB_OK) {
        db_shutdown_driver(driver);
        Vect_destroy_field_info(fi);
        return (NULL);
    }
    db_init_string(&table_name);
    db_set_string(&table_name, fi->table);
    if (db_describe_table(driver, &table_name, &table) != DB_OK) {
        goto cleanup_exit;
    }

    ncols = db_get_table_number_of_columns(table);
    col_names = G_malloc(ncols * sizeof(char *));
    for (col = 0; col < ncols; col++)
        col_names[col] = db_get_column_name(db_get_table_column(table, col));
    if ((list = G_str_concat(col_names, ncols, ",", BUFF_MAX)) == NULL)
        list = G_store("");
    G_free(col_names);
    G_debug(3, "%s", list);

cleanup_exit:
    Vect_destroy_field_info(fi);
    db_close_database_shutdown_driver(driver);

    return list;
}

/*!
   \brief Fetches list of DB column types of vector map attribute table

   \param Map vector map
   \param field layer number

   \return list of column(s) types on success
   \return NULL on error
 */
const char *Vect_get_column_types(struct Map_info *Map, int field)
{
    int num_dblinks, ncols, col;
    struct field_info *fi = NULL;
    dbDriver *driver = NULL;
    dbHandle handle;
    dbString table_name;
    dbTable *table;
    const char **sqltype_names;
    char *list = NULL;

    num_dblinks = Vect_get_num_dblinks(Map);
    if (num_dblinks <= 0)
        return (NULL);

    G_debug(3, "Displaying column types for database connection of layer %d:",
            field);
    if ((fi = Vect_get_field(Map, field)) == NULL)
        return (NULL);
    driver = db_start_driver(fi->driver);
    if (driver == NULL) {
        Vect_destroy_field_info(fi);
        return (NULL);
    }
    db_init_handle(&handle);
    db_set_handle(&handle, fi->database, NULL);
    if (db_open_database(driver, &handle) != DB_OK) {
        db_shutdown_driver(driver);
        Vect_destroy_field_info(fi);
        return (NULL);
    }
    db_init_string(&table_name);
    db_set_string(&table_name, fi->table);
    if (db_describe_table(driver, &table_name, &table) != DB_OK) {
        goto cleanup_exit;
    }

    ncols = db_get_table_number_of_columns(table);
    sqltype_names = G_malloc(ncols * sizeof(char *));
    for (col = 0; col < ncols; col++)
        sqltype_names[col] = db_sqltype_name(
            db_get_column_sqltype(db_get_table_column(table, col)));
    if ((list = G_str_concat(sqltype_names, ncols, ",", BUFF_MAX)) == NULL)
        list = G_store("");
    G_free(sqltype_names);
    G_debug(3, "%s", list);

cleanup_exit:
    Vect_destroy_field_info(fi);
    db_close_database_shutdown_driver(driver);

    return list;
}

/*!
   \brief Fetches list of DB column names and types of vector map attribute
   table

   \param Map vector map
   \param field layer number

   \return list of column(s) types on success
   \return NULL on error
 */
const char *Vect_get_column_names_types(struct Map_info *Map, int field)
{
    int num_dblinks, ncols, col;
    struct field_info *fi = NULL;
    dbDriver *driver = NULL;
    dbHandle handle;
    dbString table_name;
    dbTable *table;
    char **col_type_names;
    char *list = NULL;

    num_dblinks = Vect_get_num_dblinks(Map);
    if (num_dblinks <= 0)
        return (NULL);

    G_debug(3, "Displaying column types for database connection of layer %d:",
            field);
    if ((fi = Vect_get_field(Map, field)) == NULL)
        return (NULL);
    driver = db_start_driver(fi->driver);
    if (driver == NULL) {
        Vect_destroy_field_info(fi);
        return (NULL);
    }
    db_init_handle(&handle);
    db_set_handle(&handle, fi->database, NULL);
    if (db_open_database(driver, &handle) != DB_OK) {
        db_shutdown_driver(driver);
        Vect_destroy_field_info(fi);
        return (NULL);
    }
    db_init_string(&table_name);
    db_set_string(&table_name, fi->table);
    if (db_describe_table(driver, &table_name, &table) != DB_OK) {
        goto cleanup_exit;
    }

    ncols = db_get_table_number_of_columns(table);
    col_type_names = G_malloc(ncols * sizeof(char *));
    for (col = 0; col < ncols; col++) {
        col_type_names[col] = (char *)G_calloc(256, sizeof(char));

        snprintf(col_type_names[col], 256, "%s(%s)",
                 db_get_column_name(db_get_table_column(table, col)),
                 db_sqltype_name(
                     db_get_column_sqltype(db_get_table_column(table, col))));
    }

    if ((list = G_str_concat((const char **)col_type_names, ncols, ",",
                             BUFF_MAX)) == NULL)
        list = G_store("");

    for (col = 0; col < ncols; col++) {
        G_free(col_type_names[col]);
    }
    G_free(col_type_names);
    G_debug(3, "%s", list);

cleanup_exit:
    Vect_destroy_field_info(fi);
    db_close_database_shutdown_driver(driver);

    return list;
}
