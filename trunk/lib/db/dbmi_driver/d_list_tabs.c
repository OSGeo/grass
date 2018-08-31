/*!
 * \file db/dbmi_driver/d_list_tabs.c
 * 
 * \brief DBMI Library (driver) - list tables
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
   \brief List available tables for given connection

   \return DB_OK on success
   \return DB_FAILED on failure
 */
int db_d_list_tables(void)
{
    dbString *names;
    int count;
    int system;
    int stat;

    /* arg(s) */
    DB_RECV_INT(&system);

    /* call the procedure */
    stat = db_driver_list_tables(&names, &count, system);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* send results */
    DB_SEND_STRING_ARRAY(names, count);

    return DB_OK;
}
