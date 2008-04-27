#include "dbmi.h"
#include "macros.h"

int
db_d_begin_work()
{
    int stat;
    int result;

/* no arg(s) */

/* get the driver state and see if a database is open */
    if (!db__test_database_open())
    {
	db_error ("no database is open");
	DB_SEND_FAILURE();
	return DB_OK;
    };
/* make sure all cursors are closed */
/*
    db__close_all_cursors();
*/

/* call the procedure */
    stat = db_driver_begin_work(&result);

/* send the return code */
    if (stat != DB_OK)
    {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

/*  results */
    DB_SEND_INT (result);
    return DB_OK;
}
