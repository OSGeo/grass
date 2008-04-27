#include <grass/dbmi.h>
#include "macros.h"

/*!
 \fn int db_get_num_rows ( dbCursor *cursor )
 \brief get number of selected rows
 \return 
 \param 
*/
int
db_get_num_rows ( dbCursor *cursor )
{
    int nrows, ret_code;

    /* start the procedure call */
    db__set_protocol_fds (cursor->driver->send, cursor->driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_ROWS);

    /* send the argument(s) to the procedure */
    DB_SEND_TOKEN (&cursor->token);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return -1;

    /* get the results */
    DB_RECV_INT (&nrows);
    
    return nrows;
}
