
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
#include <stdlib.h>

#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

int db__driver_execute_immediate(dbString * sql)
{
    char *str;

    /* In addition to standard escape character ' (apostrophe) 
     * MySQL supports also \ (backslash). Because this is not SQL
     * standard, GRASS modules cannot escape all \ in the text
     * because other drivers do not support this feature. 
     * For example, if a text contains string \' GRASS modules 
     * escape ' by another ' and the string passed to the driver is \''
     * MySQL converts \' to ' but second ' remains not escaped and 
     * result is error. 
     * Because of this, all occurrences of \ in sql must be 
     * escaped by \ */
    str = G_str_replace(db_get_string(sql), "\\", "\\\\");

    G_debug(3, "Escaped SQL: %s", str);

    if (mysql_query(connection, str) != 0) {
	db_d_append_error("%s\n%s\n%s",
			  _("Unable to execute:"),
			  str,
			  mysql_error(connection));
	db_d_report_error();
	if (str)
	    G_free(str);
	return DB_FAILED;
    }

    if (str)
	G_free(str);

    return DB_OK;
}

int db__driver_begin_transaction(void)
{
    G_debug(2, "mysql: START TRANSACTION");

    if (mysql_query(connection, "START TRANSACTION") != 0) {
	db_d_append_error("%s %s",
			  _("Unable to start transaction:"),
			  mysql_error(connection));
	db_d_report_error();
	return DB_FAILED;
    }

    return DB_OK;
}

int db__driver_commit_transaction(void)
{
    G_debug(2, "mysql: COMMIT");

    if (mysql_query(connection, "COMMIT") != 0) {
	db_d_append_error("%s %s",
			  _("Unable to commit transaction:"),
			  mysql_error(connection));
	db_d_report_error();
	return DB_FAILED;
    }

    return DB_OK;
}
