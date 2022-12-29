/*!
 * \file db/dbmi_driver/d_fetch.c
 * 
 * \brief DBMI Library (driver) - fetch data
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


static int valid_cursor(dbCursor * cursor, int position);

/*!
  \brief Fetch data

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_d_fetch(void)
{
    dbToken token;
    dbCursor *cursor;
    int stat;
    int more;
    int position;

    /* get the arg(s) */
    DB_RECV_TOKEN(&token);
    DB_RECV_INT(&position);
    cursor = (dbCursor *) db_find_token(token);
    if (!valid_cursor(cursor, position)) {
	DB_SEND_FAILURE();
	return DB_FAILED;
    }

    /* call the procedure */
    stat = db_driver_fetch(cursor, position, &more);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* results */
    DB_SEND_INT(more);
    if (more) {
	DB_SEND_TABLE_DATA(cursor->table);
    }

    return DB_OK;
}


static int valid_cursor(dbCursor * cursor, int position)
{
    if (cursor == NULL)
	return 0;

    if (!db_test_cursor_type_fetch(cursor)) {
	db_error("not a fetchable cursor");
	return 0;
    }

    if (position != DB_NEXT && !db_test_cursor_mode_scroll(cursor)) {
	db_error("not a scrollable cursor");
	return 0;
    }

    return 1;
}
