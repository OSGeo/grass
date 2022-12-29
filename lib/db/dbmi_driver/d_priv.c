/*!
 * \file db/dbmi_driver/d_priv.c
 * 
 * \brief DBMI Library (driver) - privileges management
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
  \brief Grant privileges on table 
  
  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_d_grant_on_table(void)
{
    dbString tableName;
    int priv, to;
    int stat;

    db_init_string(&tableName);

    /* get the arg(s) */
    DB_RECV_STRING(&tableName);
    DB_RECV_INT(&priv);
    DB_RECV_INT(&to);

    /* call the procedure */
    stat = db_driver_grant_on_table(&tableName, priv, to);
    db_free_string(&tableName);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */

    return DB_OK;
}
