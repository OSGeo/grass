#include <grass/dbmi.h>
#include "macros.h"

/*!
 \fn int db_close_database (dbDriver *driver)
 \brief 
 \return 
 \param 
*/
int
db_close_database (dbDriver *driver)
{
    int ret_code;

/* start the procedure call */
    db__set_protocol_fds (driver->send, driver->recv);
    DB_START_PROCEDURE_CALL (DB_PROC_CLOSE_DATABASE);

/* get the return code for the procedure call */
    DB_RECV_RETURN_CODE (&ret_code);

    if (ret_code != DB_OK)
	return ret_code; /* ret_code SHOULD == DB_FAILED */

/* there is no data sent back from this procedure call */
    return DB_OK;
}
