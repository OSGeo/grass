#include <grass/dbmi.h>
#include "macros.h"

/*!
   \fn int db_list_indexes (dbDriver *driver, dbString *table_name, dbIndex **list, int *count)
   \brief 
   \return 
   \param 
 */
int
db_list_indexes(dbDriver * driver, dbString * table_name, dbIndex ** list,
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
