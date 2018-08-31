/*!
 * \file db/dbmi_client/table.c
 * 
 * \brief DBMI Library (client) - table management
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/*!
  \brief Check if table exists

  \param drvname driver name
  \param dbname database name
  \param tabname table name

  \return 1 exist
  \return 0 doesn't exist
  \return -1 error
*/
int db_table_exists(const char *drvname, const char *dbname, const char *tabname)
{
    dbDriver *driver;
    dbString *names;
    int i, count, found = 0;
    int full = 0;
    char buf[1000];
    char *bufp, *c;

    if (strchr(tabname, '.'))
	full = 1;

    driver = db_start_driver_open_database(drvname, dbname);
    if (driver == NULL) {
	G_warning(_("Unable open database <%s> by driver <%s>"), dbname,
		  drvname);
	return -1;
    }

    /* The table tabname can be either fully qualified in form table.schema,
     * or it can be only table name. If the name is fully qualified, compare whole name,
     * if it is not, compare only table names */

    /* user tables */
    if (db_list_tables(driver, &names, &count, 0) != DB_OK)
	return (-1);

    for (i = 0; i < count; i++) {
	strcpy(buf, db_get_string(&names[i]));
	bufp = buf;
	if (!full && (c = strchr(buf, '.'))) {
	    bufp = c + 1;
	}
	G_debug(2, "table = %s -> %s", buf, bufp);
	if (G_strcasecmp(tabname, bufp) == 0) {
	    found = 1;
	    break;
	}
    }
    db_free_string_array(names, count);

    if (!found) {		/* system tables */
	if (db_list_tables(driver, &names, &count, 1) != DB_OK)
	    return (-1);

	for (i = 0; i < count; i++) {
	    strcpy(buf, db_get_string(&names[i]));
	    bufp = buf;
	    if (!full && (c = strchr(buf, '.'))) {
		bufp = c + 1;
	    }
	    if (G_strcasecmp(tabname, bufp) == 0) {
		found = 1;
		break;
	    }
	}
	db_free_string_array(names, count);
    }
    db_close_database_shutdown_driver(driver);

    return (found);
}

/*!
  \brief Get number of rows of table

  \param driver db driver
  \param sql SQL statement

  \return number of records
  \return -1
*/
int db_get_table_number_of_rows(dbDriver * driver, dbString * sql)
{
    int nrows;
    dbCursor cursor;

    if (db_open_select_cursor(driver, sql, &cursor, DB_SEQUENTIAL) != DB_OK) {
	G_warning(_("Unable to open select cursor: '%s'"), db_get_string(sql));
	db_close_database_shutdown_driver(driver);
	return -1;
    }

    nrows = db_get_num_rows(&cursor);
    db_close_cursor(&cursor);

    return nrows;
}
