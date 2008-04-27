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
db_d_get_num_rows()
{
    dbToken token;
    dbCursor *cursor;
    int nrows;

    /* get the arg(s) */
    DB_RECV_TOKEN(&token);
    cursor = (dbCursor *) db_find_token(token);

    /* call the procedure */
    nrows = db_driver_get_num_rows (cursor);

    /* send the return code */
    if ( nrows < 0 )
    {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* results */
    DB_SEND_INT ( nrows );
    return DB_OK;
}

