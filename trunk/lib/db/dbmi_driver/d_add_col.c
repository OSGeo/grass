/*!
 * \file db/dbmi_driver/d_add_col.c
 * 
 * \brief DBMI Library (driver) - add column to table
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
   \brief Add column to table

   \return DB_OK on success
   \return DB_FAILED on failure
*/
int db_d_add_column(void)
{
    dbColumn column;
    dbString name;
    int stat;

    db_init_string(&name);
    db_init_column(&column);

    /* get the arg(s) */
    DB_RECV_STRING(&name);
    DB_RECV_COLUMN_DEFINITION(&column);

    /* call the procedure */
    stat = db_driver_add_column(&name, &column);
    db_free_string(&name);
    db_free_column(&column);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */

    return DB_OK;
}
