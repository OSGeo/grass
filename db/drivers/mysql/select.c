
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

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

int db__driver_open_select_cursor(dbString * sel, dbCursor * dbc, int mode)
{
    cursor *c;
    dbTable *table;
    char *str;

    init_error();

    /* allocate cursor */
    c = alloc_cursor();
    if (c == NULL)
	return DB_FAILED;

    db_set_cursor_mode(dbc, mode);
    db_set_cursor_type_readonly(dbc);

    /* \ must be escaped, see explanation in 
     * db_driver_execute_immediate() */
    str = G_str_replace(db_get_string(sel), "\\", "\\\\");
    G_debug(3, "Escaped SQL: %s", str);

    if (mysql_query(connection, str) != 0) {
	append_error(_("Cannot select data: \n"));
	append_error(db_get_string(sel));
	append_error("\n");
	append_error(mysql_error(connection));
	if (str)
	    G_free(str);
	report_error();
	return DB_FAILED;
    }

    if (str)
	G_free(str);
    c->res = mysql_store_result(connection);

    if (c->res == NULL) {
	append_error(db_get_string(sel));
	append_error("\n");
	append_error(mysql_error(connection));
	report_error();
	return DB_FAILED;
    }

    if (describe_table(c->res, &table, c) == DB_FAILED) {
	append_error("Cannot describe table\n");
	report_error();
	mysql_free_result(c->res);
	return DB_FAILED;
    }

    c->nrows = (int)mysql_num_rows(c->res);

    /* record table with dbCursor */
    db_set_cursor_table(dbc, table);

    /* set dbCursor's token for my cursor */
    db_set_cursor_token(dbc, c->token);

    return DB_OK;
}
