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
int db_d_list_indexes()
{
    dbIndex *list;
    dbString table_name;
    int count;
    int stat;

    /* arg(s) */
    db_init_string(&table_name);
    DB_RECV_STRING(&table_name);

    /* call the procedure */
    stat = db_driver_list_indexes(&table_name, &list, &count);
    db_free_string(&table_name);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* send results */
    DB_SEND_INDEX_ARRAY(list, count);
    db_free_index_array(list, count);
    return DB_OK;
}
