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
int db_d_add_column()
{
    dbColumn column;
    dbString name;
    int stat;

    db_init_string(&name);
    db_init_column(&column);

    /* get the arg(s) */
    DB_RECV_STRING(&name);
    DB_RECV_COLUMN_DEFINITION(&column);

    /* call the procedure */
    stat = db_driver_add_column(&name, &column);
    db_free_string(&name);
    db_free_column(&column);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */

    return DB_OK;
}
