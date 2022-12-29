/*!
 * \file db/dbmi_client/c_listdb.c
 * 
 * \brief DBMI Library (client) - list databases
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
  \brief List databases
  
  \param driver db driver
  \param path db path
  \param npaths number of given paths
  \param[out] handles handle infos
  \param[out] count number of handle infos

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_list_databases(dbDriver * driver, dbString * path, int npaths,
		      dbHandle ** handles, int *count)
{
    int ret_code;
    int i;
    dbHandle *h;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_LIST_DATABASES);

    /* arguments */
    DB_SEND_STRING_ARRAY(path, npaths);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* results */
    DB_RECV_INT(count);
    h = db_alloc_handle_array(*count);
    for (i = 0; i < *count; i++) {
	DB_RECV_HANDLE(&h[i]);
    }
    *handles = h;

    return DB_OK;
}
