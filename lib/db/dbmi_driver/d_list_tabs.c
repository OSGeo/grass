#include <grass/dbmi.h>
#include "macros.h"
#include "dbstubs.h"

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_d_list_tables()
{
    dbString *names;
    int count;
    int system;
    int stat;

    /* arg(s) */
    DB_RECV_INT(&system);

    /* call the procedure */
    stat = db_driver_list_tables(&names, &count, system);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* send results */
    DB_SEND_STRING_ARRAY(names, count);

    return DB_OK;
}
