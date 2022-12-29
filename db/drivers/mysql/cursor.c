
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
#include <stdio.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

int db__driver_close_cursor(dbCursor * dbc)
{
    cursor *c;

    /* get my cursor via the dbc token */
    c = (cursor *) db_find_token(db_get_cursor_token(dbc));
    if (c == NULL)
	return DB_FAILED;

    /* free_cursor(cursor) */
    free_cursor(c);

    return DB_OK;
}


cursor *alloc_cursor()
{
    cursor *c;

    /* allocate the cursor */
    c = (cursor *) db_malloc(sizeof(cursor));
    if (c == NULL) {
	db_d_append_error(_("Unable allocate cursor."));
	return NULL;
    }

    c->res = NULL;

    /* tokenize it */
    c->token = db_new_token(c);
    if (c->token < 0) {
	db_d_append_error(_("Unable to add dnew token."));
	return NULL;
    }

    c->cols = NULL;
    c->ncols = 0;

    return c;
}

void free_cursor(cursor * c)
{
    db_drop_token(c->token);

    if (c->res) {
	mysql_free_result(c->res);
    }

    G_free(c->cols);
    G_free(c);
}
