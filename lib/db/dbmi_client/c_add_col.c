/*!
 * \file db/dbmi_client/c_add_col.c
 *
 * \brief DBMI Library (client) - add column to table
 *
 * SPDX-FileCopyrightText: 1999-2008 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <grass/dbmi.h>
#include "macros.h"

/*!
   \brief Add column to table

   \param driver db driver
   \param tableName table name
   \param column new column description (dbColumn structure)

   \return DB_OK on success
   \return DB_FAILED on failure
 */
int db_add_column(dbDriver *driver, dbString *tableName, dbColumn *column)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_ADD_COLUMN);

    /* send the argument(s) to the procedure */
    DB_SEND_STRING(tableName);
    DB_SEND_COLUMN_DEFINITION(column);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
        return ret_code; /* ret_code SHOULD == DB_FAILED */

    /* no results */
    return DB_OK;
}
