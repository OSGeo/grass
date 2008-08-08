/*!
 * \file db/dbmi_driver/d_drop_index.c
 * 
 * \brief DBMI Library (driver) - drop index
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
  \brief Drop index
  
  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_d_drop_index(void)
{
    dbString name;
    int stat;

    db_init_string(&name);

    /* get the argument(s) to the procedure */
    DB_RECV_STRING(&name);

    /* call the procedure */
    stat = db_driver_drop_index(&name);

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
