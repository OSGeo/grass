/*!
 * \file db/dbmi_client/c_list_idx.c
 * 
 * \brief DBMI Library (client) - list indexes
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

/*!
  \brief List indexes

  \param driver db driver
  \param table_name table name
  \param[out] list list of db indexes
  \param[out] dbDriver number of items in the list

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_list_indexes(dbDriver * driver, dbString * table_name, dbIndex ** list,
		    int *count)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_LIST_INDEXES);

    /* arguments */
    DB_SEND_STRING(table_name);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* results */
    DB_RECV_INDEX_ARRAY(list, count);

    return DB_OK;
}
