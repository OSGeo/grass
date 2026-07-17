/*!
   \file db/drivers/cursor.c

   \brief Low level OGR SQL driver

   SPDX-FileCopyrightText: 2004-2009 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Radim Blazek
   \author Some updates by Martin Landa <landa.martin gmail.com>
 */

#include <stdio.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include <ogr_api.h>

#include "globals.h"
#include "proto.h"

/*!
   \brief Close cursor

   \param dbc pointer to dbCursor to be closed

   \return DB_OK on success
   \return DB_FAILED on failure
 */
int db__driver_close_cursor(dbCursor *dbc)
{
    cursor *c;

    G_debug(3, "db_driver_close_cursor()");

    /* get my cursor via the dbc token */
    c = (cursor *)db_find_token(db_get_cursor_token(dbc));
    if (c == NULL)
        return DB_FAILED;

    /* free_cursor(cursor) */
    free_cursor(c);

    G_debug(3, "Cursor closed");

    return DB_OK;
}

/*!
   \brief Allocate cursor

   \return pointer to cursor structure
   \return NULL on error
 */
cursor *alloc_cursor(void)
{
    cursor *c;

    /* allocate the cursor */
    c = (cursor *)db_malloc(sizeof(cursor));
    if (c == NULL) {
        db_d_append_error(_("Unable to allocate cursor"));
        return NULL;
    }

    G_zero(c, sizeof(cursor));

    /* tokenize it */
    c->token = db_new_token(c);
    if (c->token < 0) {
        db_d_append_error(_("Unable to add new token"));
        return NULL;
    }
    return c;
}

/*!
   \brief Free cursor structure (destroy OGR feature and release OGR layer)

   \param c pointer to cursor
 */
void free_cursor(cursor *c)
{
    if (c->hFeature)
        OGR_F_Destroy(c->hFeature);

    if (c->hLayer)
        OGR_DS_ReleaseResultSet(hDs, c->hLayer);

    dbToken tok = c->token;
    G_free(c->cols);
    G_free(c);

    db_drop_token(tok);
}
