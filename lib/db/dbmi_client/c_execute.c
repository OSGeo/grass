/*!
 * \file db/dbmi_client/c_execute.c
 * 
 * \brief DBMI Library (client) - execute SQL statements
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
  \brief Execute SQL statements

  \param driver db driver
  \param SQLstatement SQL statement (alter, update, ...)

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_execute_immediate(dbDriver * driver, dbString * SQLstatement)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_EXECUTE_IMMEDIATE);

    /* send the argument(s) to the procedure */
    DB_SEND_STRING(SQLstatement);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* no results */
    return DB_OK;
}

/*!
  \brief Begin transaction

  \param driver db driver

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_begin_transaction(dbDriver * driver)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_BEGIN_TRANSACTION);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;

    /* no results */
    return DB_OK;
}

/*!
  \brief Commit transaction
  
  \param driver db driver
  
  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_commit_transaction(dbDriver * driver)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_COMMIT_TRANSACTION);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;

    /* no results */
    return DB_OK;
}
