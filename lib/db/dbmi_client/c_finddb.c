#include <stdlib.h>
#include <grass/dbmi.h>
#include "macros.h"

/*!
 \fn int db_find_database (dbDriver *driver, dbHandle *handle, int *found)
 \brief 
 \return 
 \param 
*/
int
db_find_database (dbDriver *driver, dbHandle *handle, int *found)
{
    int ret_code;
    int stat;
    dbHandle temp;

/* start the procedure call */
    db__set_protocol_fds (driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_FIND_DATABASE);

/* send the arguments to the procedure */
    DB_SEND_HANDLE (handle);

/* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code; /* ret_code SHOULD == DB_FAILED */

/* get results */
    DB_RECV_INT(found);

    stat = DB_OK;
    if (*found)
    {
	DB_RECV_HANDLE (&temp);
	stat = db_set_handle (handle,
	               db_get_handle_dbname(&temp),
	               db_get_handle_dbschema(&temp)
		      );
	db_free_handle (&temp);
    }
    return stat;
}
