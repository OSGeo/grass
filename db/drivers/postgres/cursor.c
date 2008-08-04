#include <stdio.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"

int db__driver_close_cursor(dbCursor * dbc)
{
    cursor *c;

    init_error();

    /* get my cursor via the dbc token */
    c = (cursor *) db_find_token(db_get_cursor_token(dbc));
    if (c == NULL)
	return DB_FAILED;

    /* free_cursor(cursor) */
    free_cursor(c);

    return DB_OK;
}


cursor *alloc_cursor(void)
{
    cursor *c;

    /* allocate the cursor */
    c = (cursor *) db_malloc(sizeof(cursor));
    if (c == NULL) {
	append_error("Cannot allocate cursor.");
	return NULL;
    }

    c->res = NULL;

    /* tokenize it */
    c->token = db_new_token(c);
    if (c->token < 0) {
	append_error("Cannot ad new token.");
	return NULL;
    }

    c->cols = NULL;
    c->ncols = 0;

    return c;
}

void free_cursor(cursor * c)
{
    db_drop_token(c->token);

    /* TODO close results if any */

    G_free(c->cols);
    G_free(c);
}
