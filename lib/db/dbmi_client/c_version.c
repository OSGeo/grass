/*!
 * \file db/dbmi_client/c_version.c
 * 
 * \brief DBMI Library (client) - version info
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
  \brief Get version info

  Note: renamed from db_version to db_gversion to avoid name conflict
  with Berkeley DB etc.
  
  \param driver db driver
  \param[out] client_version client version
  \param[out] driver_version driver version

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_gversion(dbDriver * driver, dbString * client_version,
		dbString * driver_version)
{
    int ret_code;

    /* initialize the strings */
    db_init_string(client_version);
    db_init_string(driver_version);

    /* set client version from DB_VERSION */
    db_set_string(client_version, DB_VERSION);

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_VERSION);

    /* no arguments */

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* get the driver version */
    DB_RECV_STRING(driver_version);

    return DB_OK;
}
