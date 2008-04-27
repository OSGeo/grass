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
db_d_open_database()
{
    dbHandle handle;
    int stat;

/* get the arg(s) */
    db_init_handle (&handle);
    DB_RECV_HANDLE(&handle);
    
/* see if there is a database already open */
    if (db__test_database_open())
    {
	db_error ("Multiple open databases not allowed");
	DB_SEND_FAILURE();
	return DB_OK;
    }

/* call the procedure */
    stat = db_driver_open_database (&handle);

/* send the return code */
    if (stat != DB_OK)
    {
	db_free_handle (&handle);
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

/* record the open in the driver state */
    db__mark_database_open (
    	db_get_handle_dbname (&handle),
    	db_get_handle_dbschema (&handle));
/* DO NOT free the handle since we saved the pointers to the name,path */

/* no results */
    return DB_OK;
}
