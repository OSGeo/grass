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
db_d_drop_index (void)
{
    dbString name;
    int stat;

    db_init_string (&name);

/* get the argument(s) to the procedure */
    DB_RECV_STRING (&name);

/* call the procedure */
    stat = db_driver_drop_index (&name);

    db_free_string (&name);

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
