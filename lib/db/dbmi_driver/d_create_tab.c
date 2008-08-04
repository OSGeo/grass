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
int db_d_create_table()
{
    dbTable *table;
    int stat;

    /* get the arg(s) */
    DB_RECV_TABLE_DEFINITION(&table);

    /* call the procedure */
    stat = db_driver_create_table(table);
    db_free_table(table);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}
