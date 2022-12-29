/*!
 * \file db/dbmi_client/c_insert.c
 * 
 * \brief DBMI Library (client) - insert new record
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
  \brief Insert new record into table

  \param cursor db cursor

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_insert(dbCursor * cursor)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(cursor->driver->send, cursor->driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_INSERT);

    /* send the argument(s) to the procedure */
    DB_SEND_TOKEN(&cursor->token);
    DB_SEND_TABLE_DATA(cursor->table);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* no results */
    return DB_OK;
}
