/*!
 * \file db/dbmi_client/c_list_tabs.c
 * 
 * \brief DBMI Library (client) - list tables
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
#include <string.h>
#include <grass/dbmi.h>
#include "macros.h"

static int cmp_dbstr(const void *pa, const void *pb)
{
    const char *a = db_get_string((dbString *) pa);
    const char *b = db_get_string((dbString *) pb);

    return strcmp(a, b);
}

/*!
   \brief List available tables for given connection

   \param driver db driver
   \param[out] names list of table names
   \param[out] count number of items in the list
   \param system ?

   \return DB_OK on success
   \return DB_FAILED on failure
 */
int db_list_tables(dbDriver * driver, dbString ** names, int *count, int system)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_LIST_TABLES);

    /* arguments */
    DB_SEND_INT(system);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* results */
    DB_RECV_STRING_ARRAY(names, count);

    qsort(*names, *count, sizeof(dbString), cmp_dbstr);

    return DB_OK;
}
