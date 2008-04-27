/*!
  \file dbcolumns.c
  
  \brief Vector library - DB info on vectors maps
  
  Higher level functions for reading/writing/manipulating vectors.

  (C) 2005-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Markus Neteler
  
  \date 2005-2008
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>

/*!
  \brief Fetches list of DB column names of vector map attribute table

  \param Map vector map
  \param field layer number

  \return list of column(s) names on success
  \return NULL on error 
*/
char *Vect_get_column_names(struct Map_info *Map, int field)
{
    int num_dblinks, ncols, col;
    struct field_info *fi;
    dbDriver *driver = NULL;
    dbHandle handle;
    dbString table_name;
    dbTable *table;
    char buf[2000];
    char *ptr;

    
    num_dblinks = Vect_get_num_dblinks(Map);
    if (num_dblinks <= 0) {
	return (NULL);
    }
    else {			/* num_dblinks > 0 */

	G_debug(3,
		"Displaying column names for database connection of layer %d:",
		field);
	if ((fi = Vect_get_field(Map, field)) == NULL)
	    return (NULL);
	driver = db_start_driver(fi->driver);
	if (driver == NULL)
	    return (NULL);
	db_init_handle(&handle);
	db_set_handle(&handle, fi->database, NULL);
	if (db_open_database(driver, &handle) != DB_OK)
	    return (NULL);
	db_init_string(&table_name);
	db_set_string(&table_name, fi->table);
	if (db_describe_table(driver, &table_name, &table) != DB_OK)
	    return (NULL);

	ncols = db_get_table_number_of_columns(table);
	sprintf ( buf," " );
	for (col = 0; col < ncols; col++) {
	    if (col == 0)
		sprintf(buf, "%s", db_get_column_name(db_get_table_column(table, col)));
	    else
		sprintf(buf, "%s,%s", buf, db_get_column_name(db_get_table_column(table, col)));
	}
	G_debug(3, "%s", buf);

	db_close_database(driver);
	db_shutdown_driver(driver);

	ptr = G_malloc ( strlen(G_chop(buf)) );
        sprintf (ptr, "%s", buf);
        return (ptr);
    }
}

/*!
  \brief Fetches list of DB column types of vector map attribute table

  \param Map vector map
  \param field layer number

  \return list of column(s) types on success
  \return NULL on error 
*/
char *Vect_get_column_types(struct Map_info *Map, int field)
{
    int num_dblinks, ncols, col;
    struct field_info *fi;
    dbDriver *driver = NULL;
    dbHandle handle;
    dbString table_name;
    dbTable *table;
    char buf[2000];
    char *ptr;

    
    num_dblinks = Vect_get_num_dblinks(Map);
    if (num_dblinks <= 0) {
	return (NULL);
    }
    else {			/* num_dblinks > 0 */

	G_debug(3,
		"Displaying column types for database connection of layer %d:",
		field);
	if ((fi = Vect_get_field(Map, field)) == NULL)
	    return (NULL);
	driver = db_start_driver(fi->driver);
	if (driver == NULL)
	    return (NULL);
	db_init_handle(&handle);
	db_set_handle(&handle, fi->database, NULL);
	if (db_open_database(driver, &handle) != DB_OK)
	    return (NULL);
	db_init_string(&table_name);
	db_set_string(&table_name, fi->table);
	if (db_describe_table(driver, &table_name, &table) != DB_OK)
	    return (NULL);

	ncols = db_get_table_number_of_columns(table);
	sprintf ( buf," " );
	for (col = 0; col < ncols; col++) {
	    if (col == 0)
		sprintf(buf, "%s", db_sqltype_name(db_get_column_sqltype(db_get_table_column(table, col))));
	    else
		sprintf(buf, "%s,%s", buf, db_sqltype_name(db_get_column_sqltype(db_get_table_column(table, col))));
	}
	G_debug(3, "%s", buf);

	db_close_database(driver);
	db_shutdown_driver(driver);

	ptr = G_malloc ( strlen(G_chop(buf)) );
        sprintf (ptr, "%s", buf);
        return (ptr);
    }
}


/*!
  \brief Fetches list of DB column names and types of vector map attribute table

  \param Map vector map
  \param field layer number

  \return list of column(s) types on success
  \retutn NULL on error 
*/
char *Vect_get_column_names_types(struct Map_info *Map, int field)
{
    int num_dblinks, ncols, col;
    struct field_info *fi;
    dbDriver *driver = NULL;
    dbHandle handle;
    dbString table_name;
    dbTable *table;
    char buf[2000];
    char *ptr;

    
    num_dblinks = Vect_get_num_dblinks(Map);
    if (num_dblinks <= 0) {
	return (NULL);
    }
    else {			/* num_dblinks > 0 */

	G_debug(3,
		"Displaying column types for database connection of layer %d:",
		field);
	if ((fi = Vect_get_field(Map, field)) == NULL)
	    return (NULL);
	driver = db_start_driver(fi->driver);
	if (driver == NULL)
	    return (NULL);
	db_init_handle(&handle);
	db_set_handle(&handle, fi->database, NULL);
	if (db_open_database(driver, &handle) != DB_OK)
	    return (NULL);
	db_init_string(&table_name);
	db_set_string(&table_name, fi->table);
	if (db_describe_table(driver, &table_name, &table) != DB_OK)
	    return (NULL);

	ncols = db_get_table_number_of_columns(table);
	sprintf ( buf," " );
	for (col = 0; col < ncols; col++) {
	    if (col == 0)
		sprintf(buf, "%s(%s)", db_get_column_name(db_get_table_column(table, col)),
		        db_sqltype_name(db_get_column_sqltype(db_get_table_column(table, col))));
	    else
		sprintf(buf, "%s,%s(%s)", buf, db_get_column_name(db_get_table_column(table, col)),
		        db_sqltype_name(db_get_column_sqltype(db_get_table_column(table, col))));
	}
	G_debug(3, "%s", buf);

	db_close_database(driver);
	db_shutdown_driver(driver);

	ptr = G_malloc ( strlen(G_chop(buf)) );
        sprintf (ptr, "%s", buf);
        return (ptr);
    }
}
