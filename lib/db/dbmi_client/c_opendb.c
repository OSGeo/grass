#include <grass/dbmi.h>
#include "macros.h"

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
int
db_open_database  (dbDriver *driver, dbHandle *handle)

{
    int ret_code;

/* start the procedure call */
    db__set_protocol_fds (driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_OPEN_DATABASE);

/* send the arguments to the procedure */
    DB_SEND_HANDLE (handle);

/* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code; /* ret_code SHOULD == DB_FAILED */

/* no results */
    return DB_OK;
}
