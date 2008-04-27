#include <grass/dbmi.h>
#include "macros.h"

/*!
 \fn int db_drop_column (dbDriver *driver, dbString *tableName, dbString *columnName)
 \brief 
 \return 
 \param 
*/
int
db_drop_column (dbDriver *driver, dbString *tableName, dbString *columnName)
{
    int ret_code;

/* start the procedure call */
    db__set_protocol_fds (driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_DROP_COLUMN);

/* send the argument(s) to the procedure */
    DB_SEND_STRING (tableName);
    DB_SEND_STRING (columnName);

/* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code; /* ret_code SHOULD == DB_FAILED */

/* no results */
    return DB_OK;
}
