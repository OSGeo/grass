/*!
 * \file db/dbmi_driver/d_listdb.c
 * 
 * \brief DBMI Library (driver) - list databases
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
  \brief List databases
  
  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_d_list_databases(void)
{
    dbHandle *handles;
    dbString *path;
    int npaths;
    int i, count;
    int stat;

    /* arg(s) */
    DB_RECV_STRING_ARRAY(&path, &npaths);

    /* call the procedure */
    stat = db_driver_list_databases(path, npaths, &handles, &count);
    db_free_string_array(path, npaths);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* send results */
    DB_SEND_INT(count);
    for (i = 0; i < count; i++) {
	DB_SEND_HANDLE(&handles[i]);
    }
    db_free_handle_array(handles, count);
    return DB_OK;
}
