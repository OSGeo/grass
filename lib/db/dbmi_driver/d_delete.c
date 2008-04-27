#include <grass/dbmi.h>
#include "macros.h"
#include "dbstubs.h"

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
int
db_d_delete()
{
    dbToken token;
    dbCursor *cursor;
    int stat;

/* get the arg(s) */
    DB_RECV_TOKEN(&token);
    cursor = (dbCursor *) db_find_token(token);
    if (cursor == NULL || !db_test_cursor_type_update(cursor))
    {
	db_error ("** not an update cursor **");
	DB_SEND_FAILURE();
	return DB_FAILED;
    }

/* call the procedure */
    stat = db_driver_delete (cursor);

/* send the return code */
    if (stat != DB_OK)
    {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

/* no results */
    return DB_OK;
}
