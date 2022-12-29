/*!
 * \file db/dbmi_client/c_openupdate.c
 * 
 * \brief DBMI Library (client) - open update cursor
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

/*!
  \brief Open update cursor

  \param driver db driver
  \param table_name table name
  \param select SQL update statement (?)
  \param cursor db cursor to be opened
  \param mode open mode (?)

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_open_update_cursor(dbDriver * driver, dbString * table_name,
			  dbString * select, dbCursor * cursor, int mode)
{
    int ret_code;

    db_init_cursor(cursor);
    cursor->driver = driver;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_OPEN_UPDATE_CURSOR);

    /* send the argument(s) to the procedure */
    DB_SEND_STRING(table_name);
    DB_SEND_STRING(select);
    DB_SEND_INT(mode);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* get the results */
    DB_RECV_TOKEN(&cursor->token);
    DB_RECV_INT(&cursor->type);
    DB_RECV_INT(&cursor->mode);
    DB_RECV_TABLE_DEFINITION(&cursor->table);
    db_alloc_cursor_column_flags(cursor);
    return DB_OK;
}
