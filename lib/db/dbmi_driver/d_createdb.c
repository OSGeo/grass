/*!
 * \file db/dbmi_driver/d_createdb.c
 * 
 * \brief DBMI Library (driver) - create database
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
