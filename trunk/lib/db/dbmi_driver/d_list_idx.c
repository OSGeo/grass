/*!
 * \file db/dbmi_driver/d_list_idx.c
 * 
 * \brief DBMI Library (driver) - list indexes
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
  \brief List indexes

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_d_list_indexes(void)
{
    dbIndex *list;
    dbString table_name;
    int count;
    int stat;

    /* arg(s) */
    db_init_string(&table_name);
    DB_RECV_STRING(&table_name);

    /* call the procedure */
    stat = db_driver_list_indexes(&table_name, &list, &count);
    db_free_string(&table_name);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* send results */
    DB_SEND_INDEX_ARRAY(list, count);
    db_free_index_array(list, count);
    return DB_OK;
}
