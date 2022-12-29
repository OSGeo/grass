
/*****************************************************************************
*
* MODULE:       DBF driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      Simple driver for reading and writing dbf files     
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

#include <grass/dbmi.h>
#include <grass/shapefil.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

int db__driver_open_select_cursor(dbString * sel, dbCursor * dbc, int mode)
{
    int ret;
    cursor *c;
    char *sql;
    dbTable *table;

    /* allocate cursor */
    c = alloc_cursor();
    if (c == NULL)
	return DB_FAILED;

    db_set_cursor_mode(dbc, mode);
    db_set_cursor_type_readonly(dbc);

    sql = db_get_string(sel);

    ret = execute(sql, c);

    if (ret == DB_FAILED) {
	db_d_append_error(_("Unable to open cursor."));
	db_d_report_error();
	return DB_FAILED;
    }

    describe_table(c->table, c->cols, c->ncols, &table);

    /* record table with dbCursor */
    db_set_cursor_table(dbc, table);

    /* set dbCursor's token for my cursor */
    db_set_cursor_token(dbc, c->token);

    return DB_OK;
}
