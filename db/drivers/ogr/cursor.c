
/*****************************************************************************
*
* MODULE:       OGR driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      DB driver for OGR sources     
*
* COPYRIGHT:    (C) 2004 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <stdio.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include "ogr_api.h"
#include "globals.h"
#include "proto.h"

int db__driver_close_cursor(dbCursor * dbc)
{
    cursor *c;

    G_debug(3, "db_driver_close_cursor()");

    init_error();

    /* get my cursor via the dbc token */
    c = (cursor *) db_find_token(db_get_cursor_token(dbc));
    if (c == NULL)
	return DB_FAILED;

    /* free_cursor(cursor) */
    free_cursor(c);

    G_debug(3, "Cursor closed");

    return DB_OK;
}


cursor *alloc_cursor()
{
    cursor *c;

    /* allocate the cursor */
    c = (cursor *) db_malloc(sizeof(cursor));
    if (c == NULL) {
	append_error("Cannot allocate cursor.");
	return NULL;
    }

    /* tokenize it */
    c->token = db_new_token(c);
    if (c->token < 0) {
	append_error("Cannot ad new token.");
	return NULL;
    }

    c->hFeature = NULL;

    return c;
}

void free_cursor(cursor * c)
{
    if (c->hFeature)
	OGR_F_Destroy(c->hFeature);

    if (c->hLayer)
	OGR_DS_ReleaseResultSet(hDs, c->hLayer);

    G_free(c->cols);
    G_free(c);

    db_drop_token(c->token);
}
