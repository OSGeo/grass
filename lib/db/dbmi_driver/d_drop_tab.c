/*!
 * \file db/dbmi_driver/d_drop_tab.c
 *
 * \brief DBMI Library (driver) - drop table
 *
 * SPDX-FileCopyrightText:  1999-2008 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <stdlib.h>
#include <grass/dbmi.h>
#include "macros.h"
#include "dbstubs.h"

/*!
   \brief Drop table

   \return DB_OK on success
   \return DB_FAILED on failure
 */
int db_d_drop_table(void)
{
    dbString name;
    int stat;

    db_init_string(&name);

    /* get the argument(s) to the procedure */
    DB_RECV_STRING(&name);

    /* call the procedure */
    stat = db_driver_drop_table(&name);

    db_free_string(&name);

    /* send the return code */
    if (stat != DB_OK) {
        DB_SEND_FAILURE();
        return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}
