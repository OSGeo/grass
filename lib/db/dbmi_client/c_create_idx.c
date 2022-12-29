/*!
 * \file db/dbmi_client/c_create_idx.c
 * 
 * \brief DBMI Library (client) - create index
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <string.h>
#include <grass/dbmi.h>
#include "macros.h"

/*!
  \brief Create index

  \param driver db driver
  \param index index info (pointer to dbIndex structure)

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_create_index(dbDriver * driver, dbIndex * index)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_CREATE_INDEX);

    /* send the arguments to the procedure */
    DB_SEND_INDEX(index);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* get results */
    DB_RECV_STRING(&index->indexName);

    return DB_OK;
}

/*!
  \brief Create unique index

  \param driver db driver
  \param table_name table name
  \param column_name column name (where to create index)

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_create_index2(dbDriver * driver, const char *table_name,
		     const char *column_name)
{
    int ret;
    dbIndex index;
    char buf[1000];
    const char *tbl;

    db_init_index(&index);
    db_alloc_index_columns(&index, 1);

    tbl = strchr(table_name, '.');
    if (tbl == NULL)
	tbl = table_name;
    else
	tbl++;

    sprintf(buf, "%s_%s", tbl, column_name);
    db_set_index_name(&index, buf);

    db_set_index_table_name(&index, table_name);
    db_set_index_column_name(&index, 0, column_name);
    db_set_index_type_unique(&index);

    ret = db_create_index(driver, &index);

    db_free_index(&index);

    return ret;
}
