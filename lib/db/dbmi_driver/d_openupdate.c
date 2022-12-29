/*!
 * \file db/dbmi_driver/d_openupdate.c
 * 
 * \brief DBMI Library (driver) - open update cursor
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
  \brief Open update cursor

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_d_open_update_cursor(void)
{
    dbCursor *cursor;
    int stat;
    dbToken token;
    dbString select;
    dbString table_name;
    int mode;

    /* get the arg(s) */
    db_init_string(&table_name);
    db_init_string(&select);
    DB_RECV_STRING(&table_name);
    DB_RECV_STRING(&select);
    DB_RECV_INT(&mode);

    /* create a cursor */
    cursor = (dbCursor *) db_malloc(sizeof(dbCursor));
    if (cursor == NULL)
	return db_get_error_code();
    token = db_new_token((dbAddress) cursor);
    if (token < 0)
	return db_get_error_code();
    db_init_cursor(cursor);

    /* call the procedure */
    stat = db_driver_open_update_cursor(&table_name, &select, cursor, mode);
    db_free_string(&table_name);
    db_free_string(&select);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* mark this as an update cursor */
    db_set_cursor_type_update(cursor);

    /* add this cursor to the cursors managed by the driver state */
    db__add_cursor_to_driver_state(cursor);

    /* results */
    DB_SEND_TOKEN(&token);
    DB_SEND_INT(cursor->type);
    DB_SEND_INT(cursor->mode);
    DB_SEND_TABLE_DEFINITION(cursor->table);
    return DB_OK;
}
