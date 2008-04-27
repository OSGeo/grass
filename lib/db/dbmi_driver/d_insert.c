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
db_d_insert()
{
    dbToken token;
    dbCursor *cursor;
    int stat;

/* get the arg(s) */
    DB_RECV_TOKEN(&token);
    cursor = (dbCursor *) db_find_token(token);
    if (cursor == NULL || !db_test_cursor_type_insert(cursor))
    {
	db_error ("** not an insert cursor **");
	DB_SEND_FAILURE();
	return DB_FAILED;
    }
    DB_RECV_TABLE_DATA (cursor->table);

/* call the procedure */
    stat = db_driver_insert (cursor);

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
