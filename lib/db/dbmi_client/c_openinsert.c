#include <grass/dbmi.h>
#include "macros.h"

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
int
db_open_insert_cursor  (dbDriver *driver, dbCursor *cursor)

{
    int ret_code;

    /*
    db_init_cursor (cursor);
    */
    cursor->driver = driver;

/* start the procedure call */
    db__set_protocol_fds (driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_OPEN_INSERT_CURSOR);

/* send the argument(s) to the procedure */
    DB_SEND_TABLE_DEFINITION (db_get_cursor_table(cursor));

/* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code; /* ret_code SHOULD == DB_FAILED */

/* get the results */
    DB_RECV_TOKEN(&cursor->token);
    DB_RECV_INT(&cursor->type);
    DB_RECV_INT(&cursor->mode);
    return DB_OK;
}
