/*!
 * \file db/dbmi_client/c_closedb.c
 * 
 * \brief DBMI Library (client) - close database connection
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
  \brief Close database connection

  \param driver db driver

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_close_database(dbDriver * driver)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_CLOSE_DATABASE);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* there is no data sent back from this procedure call */
    return DB_OK;
}
