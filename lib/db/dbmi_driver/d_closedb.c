/*!
 * \file db/dbmi_driver/d_closedb.c
 * 
 * \brief DBMI Library (driver) - close database connection
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
#include "dbstubs.h"

/*!
  \brief Close database connection

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_d_close_database(void)
{
    int stat;

    /* no arg(s) */

    /* see if a database is open */
    if (!db__test_database_open()) {
	db_error("no database is open");
	DB_SEND_FAILURE();
	return DB_OK;
    };
    /* make sure all cursors are closed */
    db__close_all_cursors();

    /* call the procedure */
    stat = db_driver_close_database();

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* clear the driver state */
    db__mark_database_closed();
    db__init_driver_state();

    /* no results */
    return DB_OK;
}
