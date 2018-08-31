/*!
 * \file db/dbmi_driver/d_close_cur.c
 * 
 * \brief DBMI Library (driver) - close cursor
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
#include <grass/dbmi.h>
#include "macros.h"
#include "dbstubs.h"

/*!
  \brief Close cursor
  
  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_d_close_cursor(void)
{
    dbCursor *cursor;
    dbToken token;
    int stat;

    /* get the arg(s) */
    DB_RECV_TOKEN(&token);
    cursor = (dbCursor *) db_find_token(token);
    if (cursor == NULL) {
	db_error("** invalid cursor **");
	return DB_FAILED;
    }

    /* call the procedure */
    stat = db_driver_close_cursor(cursor);

    /* get rid of the cursor */
    db_drop_token(token);
    db_free_cursor(cursor);
    db__drop_cursor_from_driver_state(cursor);
    db_free(cursor); /* ?? */

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}
