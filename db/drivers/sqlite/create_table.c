/**
 * \file create_table.c
 *
 * \brief Low level SQLite table creation.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 *
 * \date 2005-2007
 */

#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"


/**
 * \fn int db__driver_create_table (dbTable *table)
 *
 * \brief SQLite create table.
 *
 * \param[in] table
 * \return int DB_FAILED on error; DB_OK on success
 */

int
db__driver_create_table (dbTable *table)
{
    int col, ncols;
    dbColumn   *column;
    char *colname;
    int sqltype;
    sqlite3_stmt *statement;
    dbString sql;
    const char *rest;
    int   ret;
    
    G_debug (3, "db__driver_create_table()");
    
    init_error();

    db_init_string (&sql);

    /* db_table_to_sql ( table, &sql ); */

    db_set_string ( &sql, "create table ");
    db_append_string ( &sql, db_get_table_name ( table ) );
    db_append_string ( &sql, " ( ");

    ncols = db_get_table_number_of_columns(table);

    for ( col = 0; col < ncols; col++ ) {
        column = db_get_table_column (table, col);
	colname = db_get_column_name (column);
	sqltype = db_get_column_sqltype (column);
	
	G_debug ( 3, "%s (%s)", colname, db_sqltype_name(sqltype) );

	if ( col > 0 ) db_append_string ( &sql, ", " );
	db_append_string ( &sql, colname );
	db_append_string ( &sql, " " );
	switch ( sqltype ) {
	    case DB_SQL_TYPE_CHARACTER:
	    case DB_SQL_TYPE_TEXT:
		db_append_string ( &sql, "text");
                break;
	    case DB_SQL_TYPE_SMALLINT:
	    case DB_SQL_TYPE_INTEGER:
		db_append_string ( &sql, "integer");
		break;
	    case DB_SQL_TYPE_REAL:
	    case DB_SQL_TYPE_DOUBLE_PRECISION:
	    case DB_SQL_TYPE_DECIMAL:
	    case DB_SQL_TYPE_NUMERIC:
	    case DB_SQL_TYPE_INTERVAL:
		db_append_string ( &sql, "real");
		break;
	    case DB_SQL_TYPE_DATE:
	    case DB_SQL_TYPE_TIME:
	    case DB_SQL_TYPE_TIMESTAMP:
		db_append_string ( &sql, "text");
		break;
 	    default:
                G_warning ( "Unknown column type (%s)", colname);
		return DB_FAILED;
	}
    }
    db_append_string ( &sql, " )" );

    G_debug (3, " SQL: %s", db_get_string(&sql) );
    
    ret = sqlite3_prepare ( sqlite, db_get_string(&sql), -1,
                            &statement, &rest );

    if ( ret != SQLITE_OK ) {
        append_error( "Cannot create table:\n");
        append_error( db_get_string(&sql) );
        append_error( "\n" );
        append_error ((char *) sqlite3_errmsg (sqlite));
        report_error();
        sqlite3_finalize ( statement );
        db_free_string ( &sql);
        return DB_FAILED;
    }

    ret = sqlite3_step ( statement );

    if ( ret != SQLITE_DONE )
    {
        append_error("Error in sqlite3_step():\n");
        append_error ((char *) sqlite3_errmsg (sqlite));
        report_error( );
        return DB_FAILED;
    }
        
    sqlite3_finalize ( statement );
    db_free_string ( &sql);
    
    return DB_OK;
}
