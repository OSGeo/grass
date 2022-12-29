/*!
 * \file db/dbmi_driver/d_rows.c
 * 
 * \brief DBMI Library (driver) - get number of records
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
  \brief Get number of selected rows

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_d_get_num_rows(void)
{
    dbToken token;
    dbCursor *cursor;
    int nrows;

    /* get the arg(s) */
    DB_RECV_TOKEN(&token);
    cursor = (dbCursor *) db_find_token(token);

    /* call the procedure */
    nrows = db_driver_get_num_rows(cursor);

    /* send the return code */
    if (nrows < 0) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* results */
    DB_SEND_INT(nrows);
    return DB_OK;
}
