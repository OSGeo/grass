/*!
 * \file db/dbmi_client/c_fetch.c
 * 
 * \brief DBMI Library (client) - fetch data
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
  \brief Fetch data from open cursor

  \param cursor pointer to dbCursor
  \param position cursor position
  \param[out] more get more (0 for no data to be fetched)

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_fetch(dbCursor * cursor, int position, int *more)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(cursor->driver->send, cursor->driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_FETCH);

    /* send the argument(s) to the procedure */
    DB_SEND_TOKEN(&cursor->token);
    DB_SEND_INT(position);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* get the results */
    DB_RECV_INT(more);
    if (*more) {
	DB_RECV_TABLE_DATA(cursor->table);
    }
    return DB_OK;
}
