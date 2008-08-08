/*!
 * \file db/dbmi_driver/d_delete.c
 * 
 * \brief DBMI Library (driver) - delete record
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <grass/dbmi.h>
#include "macros.h"
#include "dbstubs.h"

/*!
  \brief Delete record (?)

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_d_delete(void)
{
    dbToken token;
    dbCursor *cursor;
    int stat;

    /* get the arg(s) */
    DB_RECV_TOKEN(&token);
    cursor = (dbCursor *) db_find_token(token);
    if (cursor == NULL || !db_test_cursor_type_update(cursor)) {
	db_error("** not an update cursor **");
	DB_SEND_FAILURE();
	return DB_FAILED;
    }

    /* call the procedure */
    stat = db_driver_delete(cursor);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}
