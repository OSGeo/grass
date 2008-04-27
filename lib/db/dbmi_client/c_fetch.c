#include <grass/dbmi.h>
#include "macros.h"

/*!
 \fn int db_fetch (dbCursor *cursor, int position, int *more)
 \brief 
 \return 
 \param 
*/
int
db_fetch (dbCursor *cursor, int position, int *more)
{
    int ret_code;

/* start the procedure call */
    db__set_protocol_fds (cursor->driver->send, cursor->driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_FETCH);

/* send the argument(s) to the procedure */
    DB_SEND_TOKEN (&cursor->token);
    DB_SEND_INT (position);

/* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code; /* ret_code SHOULD == DB_FAILED */

/* get the results */
    DB_RECV_INT (more);
    if (*more)
    {
	DB_RECV_TABLE_DATA(cursor->table);
    }
    return DB_OK;
}
