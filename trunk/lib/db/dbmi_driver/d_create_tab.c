/*!
 * \file db/dbmi_driver/d_create_tab.c
 * 
 * \brief DBMI Library (driver) - create table
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
  \brief Create table

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_d_create_table(void)
{
    dbTable *table;
    int stat;

    /* get the arg(s) */
    DB_RECV_TABLE_DEFINITION(&table);

    /* call the procedure */
    stat = db_driver_create_table(table);
    db_free_table(table);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}
