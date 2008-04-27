#include <stdlib.h>
#include <string.h>
#include <grass/dbmi.h>
#include "macros.h"

static int cmp_dbstr(const void *pa, const void *pb)
{
    const char *a = db_get_string((dbString *)pa);
    const char *b = db_get_string((dbString *)pb);
    return strcmp(a, b);
}

/*!
 \fn int db_list_tables (dbDriver *driver, dbString **names, int *count, int system)
 \brief list available tables for given connection
 \return names of tables, success: DB_OK; DB_FAILED otherwise
 \param names: names of tables
 \param
 \param
*/
int
db_list_tables (dbDriver *driver, dbString **names, int *count, int system)
{
    int ret_code;

/* start the procedure call */
    db__set_protocol_fds (driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_LIST_TABLES);

/* arguments */
    DB_SEND_INT (system);

/* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code; /* ret_code SHOULD == DB_FAILED */

/* results */
    DB_RECV_STRING_ARRAY (names, count);

    qsort(*names, *count, sizeof(dbString), cmp_dbstr);

    return DB_OK;
}
