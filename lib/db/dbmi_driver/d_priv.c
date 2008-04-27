#include <stdlib.h>
#include <grass/dbmi.h>
#include "macros.h"
#include "dbstubs.h"

/*!
 \brief Grant privileges on table 
 \return 
 \param 
*/
int
db_d_grant_on_table()
{
    dbString tableName;
    int priv, to;
    int stat;

    db_init_string (&tableName);

/* get the arg(s) */
    DB_RECV_STRING(&tableName);
    DB_RECV_INT(&priv);
    DB_RECV_INT(&to);

/* call the procedure */
    stat = db_driver_grant_on_table ( &tableName, priv, to);
    db_free_string (&tableName);

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
