/*!
 * \file db/dbmi_driver/d_drop_col.c
 * 
 * \brief DBMI Library (driver) - drop column
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
  \brief Drop column

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_d_drop_column(void)
{
    dbString tableName;
    dbString columnName;
    int stat;

    db_init_string(&tableName);
    db_init_string(&columnName);

    /* get the arg(s) */
    DB_RECV_STRING(&tableName);
    DB_RECV_STRING(&columnName);

    /* call the procedure */
    stat = db_driver_drop_column(&tableName, &columnName);
    db_free_string(&tableName);
    db_free_string(&columnName);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */

    return DB_OK;
}
