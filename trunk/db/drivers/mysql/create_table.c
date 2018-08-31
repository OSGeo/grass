
/**********************************************************
 * MODULE:    mysql
 * AUTHOR(S): Radim Blazek (radim.blazek@gmail.com)
 * PURPOSE:   MySQL database driver
 * COPYRIGHT: (C) 2001 by the GRASS Development Team
 *            This program is free software under the 
 *            GNU General Public License (>=v2). 
 *            Read the file COPYING that comes with GRASS
 *            for details.
 **********************************************************/
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

int db__driver_create_table(dbTable * table)
{
    int col, ncols;
    dbColumn *column;
    const char *colname;
    int sqltype;
    char buf[500];
    dbString sql;

    /* dbConnection conn_par; */

    G_debug(3, "db__driver_create_table()");

    db_init_string(&sql);

    db_set_string(&sql, "CREATE TABLE ");
    db_append_string(&sql, db_get_table_name(table));
    db_append_string(&sql, " ( ");

    ncols = db_get_table_number_of_columns(table);

    for (col = 0; col < ncols; col++) {
	column = db_get_table_column(table, col);
	colname = db_get_column_name(column);
	sqltype = db_get_column_sqltype(column);

	G_debug(3, "%s (%s)", colname, db_sqltype_name(sqltype));

	if (col > 0)
	    db_append_string(&sql, ", ");
	db_append_string(&sql, colname);
	db_append_string(&sql, " ");
	switch (sqltype) {
	case DB_SQL_TYPE_SMALLINT:
	    db_append_string(&sql, "SMALLINT");
	    break;
	case DB_SQL_TYPE_INTEGER:
	    db_append_string(&sql, "INT");
	    break;

	case DB_SQL_TYPE_REAL:
	    db_append_string(&sql, "FLOAT");
	    break;

	    /* TODO: better numeric types */
	case DB_SQL_TYPE_DOUBLE_PRECISION:
	case DB_SQL_TYPE_DECIMAL:
	case DB_SQL_TYPE_NUMERIC:
	case DB_SQL_TYPE_INTERVAL:
	    db_append_string(&sql, "DOUBLE");
	    break;

	    /* GRASS does not distinguish TIMESTAMP and DATETIME */
	    /*
	       case DB_SQL_TYPE_DATETIME|DB_DATETIME_MASK:
	       db_append_string ( &sql, "DATETIME");
	       break;
	     */
	case DB_SQL_TYPE_TIMESTAMP:
	    /* db_append_string ( &sql, "TIMESTAMP"); */
	    db_append_string(&sql, "DATETIME");
	    break;

	case DB_SQL_TYPE_DATE:
	    db_append_string(&sql, "DATE");
	    break;
	case DB_SQL_TYPE_TIME:
	    db_append_string(&sql, "TIME");
	    break;

	case DB_SQL_TYPE_CHARACTER:
	    sprintf(buf, "VARCHAR(%d)", db_get_column_length(column));
	    db_append_string(&sql, buf);
	    break;
	case DB_SQL_TYPE_TEXT:
	    db_append_string(&sql, "TEXT");
	    break;

	default:
	    G_warning("Unknown column type (%s)", colname);
	    return DB_FAILED;
	}
    }
    db_append_string(&sql, " )");

    G_debug(3, " SQL: %s", db_get_string(&sql));

    if (mysql_query(connection, db_get_string(&sql)) != 0) {
	db_d_append_error("%s\n%s\%s",
			  _("Unable to create table:"),
			  db_get_string(&sql),
			  mysql_error(connection));
	db_d_report_error();
	db_free_string(&sql);
	return DB_FAILED;
    }

    /* Grant privileges */

    /*
     * 1) MySQL does not support user groups but it is possible 
     *    to specify list of users. 
     * 2) Only root can grant privileges.
     */
    /*
       db_get_connection(&conn_par);

       if ( conn_par.group ) 
       {
       db_set_string ( &sql, "GRANT SELECT ON on " );
       db_append_string ( &sql, db_get_table_name ( table ) );
       db_append_string ( &sql, " TO " );
       db_append_string ( &sql, conn_par.group );

       G_debug (3, " SQL: %s", db_get_string(&sql) );

       if ( mysql_query ( connection, db_get_string(&sql) ) != 0 )
       {
       G_warning ( "Cannot grant select on table: \n%s\n%s",
       db_get_string(&sql), mysql_error(connection) );
       }
       }
     */

    db_free_string(&sql);

    return DB_OK;
}
