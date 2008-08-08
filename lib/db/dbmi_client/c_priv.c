/*!
 * \file db/dbmi_client/c_priv.c
 * 
 * \brief DBMI Library (client) - privileges management
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
  \brief Grant privileges on table

  \param driver db driver
  \param tableName table name
  \param priv privileges DB_PRIV_SELECT
  \param to grant to DB_GROUP | DB_PUBLIC

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_grant_on_table(dbDriver * driver, const char *tableName, int priv, int to)
{
    int ret_code;
    dbString name;

    db_init_string(&name);
    db_set_string(&name, tableName);

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_GRANT_ON_TABLE);

    /* send the argument(s) to the procedure */
    DB_SEND_STRING(&name);
    DB_SEND_INT(priv);
    DB_SEND_INT(to);

    db_free_string(&name);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* no results */
    return DB_OK;
}
