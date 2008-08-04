#include <stdlib.h>
#include <grass/dbmi.h>
#include "macros.h"
#include "dbstubs.h"

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_d_close_cursor()
{
    dbCursor *cursor;
    dbToken token;
    int stat;

    /* get the arg(s) */
    DB_RECV_TOKEN(&token);
    cursor = (dbCursor *) db_find_token(token);
    if (cursor == NULL) {
	db_error("** invalid cursor **");
	return DB_FAILED;
    }

    /* call the procedure */
    stat = db_driver_close_cursor(cursor);

    /* get rid of the cursor */
    db_drop_token(token);
    db_free_cursor(cursor);
    db__drop_cursor_from_driver_state(cursor);
    free(cursor);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}
