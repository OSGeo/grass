/*!
 * \file db/dbmi_driver/d_openinsert.c
 * 
 * \brief DBMI Library (driver) - open insert cursor
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
  \brief Open insert cursor

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_d_open_insert_cursor(void)
{
    dbCursor *cursor;
    dbTable *table;
    int stat;
    dbToken token;

    /* get the arg(s) */
    DB_RECV_TABLE_DEFINITION(&table);

    /* create a cursor */
    cursor = (dbCursor *) db_malloc(sizeof(dbCursor));
    if (cursor == NULL)
	return db_get_error_code();
    token = db_new_token((dbAddress) cursor);
    if (token < 0)
	return db_get_error_code();
    db_init_cursor(cursor);
    db_set_cursor_table(cursor, table);

    /* call the procedure */
    stat = db_driver_open_insert_cursor(cursor);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* mark this as an insert cursor */
    db_set_cursor_type_insert(cursor);

    /* add this cursor to the cursors managed by the driver state */
    db__add_cursor_to_driver_state(cursor);

    /* results */
    DB_SEND_TOKEN(&token);
    DB_SEND_INT(cursor->type);
    DB_SEND_INT(cursor->mode);
    return DB_OK;
}
