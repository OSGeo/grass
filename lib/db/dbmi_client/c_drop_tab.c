/*!
  \file db/dbmi_client/c_drop_tab.c
 
 \brief DBMI Library (client) - drop table
 
 (C) 1999-2008, 2011 by the GRASS Development Team
 
 This program is free software under the GNU General Public
 License (>=v2). Read the file COPYING that comes with GRASS
 for details.

 \author Joel Jones (CERL/UIUC)
 \author Radim Blazek
*/

#include <grass/dbmi.h>
#include "macros.h"

/*!
  \brief Drop table

  \param driver db driver
  \param name table name to be dropped

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_drop_table(dbDriver * driver, dbString * name)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_DROP_TABLE);

    /* send the argument(s) to the procedure */
    DB_SEND_STRING(name);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* no results */
    return DB_OK;
}
