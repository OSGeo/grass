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
int
db_d_drop_column()
{
    dbString tableName;
    dbString columnName;
    int stat;

    db_init_string (&tableName);
    db_init_string (&columnName);

/* get the arg(s) */
    DB_RECV_STRING(&tableName);
    DB_RECV_STRING(&columnName);

/* call the procedure */
    stat = db_driver_drop_column (&tableName, &columnName);
    db_free_string (&tableName);
    db_free_string (&columnName);

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
