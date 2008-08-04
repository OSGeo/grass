#include <grass/dbmi.h>
#include "macros.h"

/*!
   \fn int db_list_databases (dbDriver *driver, dbString *path, int npaths, dbHandle **handles, int *count)
   \brief 
   \return 
   \param 
 */
int
db_list_databases(dbDriver * driver, dbString * path, int npaths,
		  dbHandle ** handles, int *count)
{
    int ret_code;
    int i;
    dbHandle *h;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_LIST_DATABASES);

    /* arguments */
    DB_SEND_STRING_ARRAY(path, npaths);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* results */
    DB_RECV_INT(count);
    h = db_alloc_handle_array(*count);
    for (i = 0; i < *count; i++) {
	DB_RECV_HANDLE(&h[i]);
    }
    *handles = h;

    return DB_OK;
}
