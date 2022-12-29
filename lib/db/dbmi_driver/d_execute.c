/*!
 * \file db/dbmi_driver/d_execute.c
 * 
 * \brief DBMI Library (driver) - execute SQL statements
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
#include "dbstubs.h"

/*!
  \brief Execute SQL statements

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_d_execute_immediate(void)
{
    int stat;
    dbString SQLstatement;

    /* get the arg(s) */
    db_init_string(&SQLstatement);
    DB_RECV_STRING(&SQLstatement);

    /* call the procedure */
    stat = db_driver_execute_immediate(&SQLstatement);
    db_free_string(&SQLstatement);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}

/*!
  \brief Begin transaction

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_d_begin_transaction(void)
{
    int stat;

    /* call the procedure */
    stat = db_driver_begin_transaction();

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}

/*!
  \brief Commit transaction
  
  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_d_commit_transaction()
{
    int stat;

    /* call the procedure */
    stat = db_driver_commit_transaction();

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}
