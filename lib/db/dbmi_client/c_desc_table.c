/*!
 * \file db/dbmi_client/c_desc_table.c
 *
 * \brief DBMI Library (client) - describe table
 *
 * SPDX-FileCopyrightText:  1999-2008 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <grass/dbmi.h>
#include "macros.h"

/*!
   \brief Describe table

   \param driver db driver
   \param name table name
   \param[out] table pointer to dbTable structure

   \return DB_OK on success
   \return DB_FAILED on failure
 */
int db_describe_table(dbDriver *driver, dbString *name, dbTable **table)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_DESCRIBE_TABLE);

    /* send the argument(s) to the procedure */
    DB_SEND_STRING(name);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
        return ret_code; /* ret_code SHOULD == DB_FAILED */

    /* get the results */
    DB_RECV_TABLE_DEFINITION(table);
    return DB_OK;
}
