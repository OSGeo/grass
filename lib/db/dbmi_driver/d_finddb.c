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
db_d_find_database()
{
    dbHandle handle;
    int found;
    int stat;

/* get the arg(s) */
    db_init_handle (&handle);
    DB_RECV_HANDLE(&handle);
    
/* call the procedure */
    stat = db_driver_find_database (&handle, &found);


/* send the return code */
    if (stat != DB_OK)
    {
	db_free_handle (&handle);
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

/* send results */
    DB_SEND_INT(found);
    if (found)
    {
	DB_SEND_HANDLE (&handle);
    }
    db_free_handle (&handle);
    return DB_OK;
}
