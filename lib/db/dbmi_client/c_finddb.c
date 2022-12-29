/*!
 * \file db/dbmi_client/c_finddb.c
 * 
 * \brief DBMI Library (client) - find database
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

/*!
  \brief Find database

  \param driver db driver
  \param handle handle info
  \param[out] found if non-zero database found

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_find_database(dbDriver * driver, dbHandle * handle, int *found)
{
    int ret_code;
    int stat;
    dbHandle temp;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_FIND_DATABASE);

    /* send the arguments to the procedure */
    DB_SEND_HANDLE(handle);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* get results */
    DB_RECV_INT(found);

    stat = DB_OK;
    if (*found) {
	DB_RECV_HANDLE(&temp);
	stat = db_set_handle(handle,
			     db_get_handle_dbname(&temp),
			     db_get_handle_dbschema(&temp)
	    );
	db_free_handle(&temp);
    }
    return stat;
}
