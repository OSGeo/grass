/*!
 * \file db/dbmi_driver/d_create_idx.c
 * 
 * \brief DBMI Library (driver) - create index
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
  \brief Create index

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_d_create_index(void)
{
    dbIndex index;
    int stat;

    /* get the arg(s) */
    db_init_index(&index);
    DB_RECV_INDEX(&index);

    /* call the procedure */
    stat = db_driver_create_index(&index);

    /* send the return code */
    if (stat != DB_OK) {
	db_free_index(&index);
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* send results */
    DB_SEND_STRING(&index.indexName);
    db_free_index(&index);
    return DB_OK;
}
