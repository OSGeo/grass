/*!
 * \file db/dbmi_driver/d_createdb.c
 *
 * \brief DBMI Library (driver) - create database
 *
 * SPDX-FileCopyrightText: 1999-2008 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <stdlib.h>
#include <grass/dbmi.h>
#include "macros.h"
#include "dbstubs.h"

/*!
   \brief Create database

   \return DB_OK on success
   \return DB_FAILED on failure
 */
int db_d_create_database(void)
{
    dbHandle handle;
    int stat;

    /* get the arg(s) */
    db_init_handle(&handle);
    DB_RECV_HANDLE(&handle);

    /* call the procedure */
    stat = db_driver_create_database(&handle);
    db_free_handle(&handle);

    /* send the return code */
    if (stat != DB_OK) {
        DB_SEND_FAILURE();
        return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}
