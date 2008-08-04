#include <grass/dbmi.h>
#include "macros.h"
#include "dbstubs.h"

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_d_close_database()
{
    int stat;

    /* no arg(s) */

    /* see if a database is open */
    if (!db__test_database_open()) {
	db_error("no database is open");
	DB_SEND_FAILURE();
	return DB_OK;
    };
    /* make sure all cursors are closed */
    db__close_all_cursors();

    /* call the procedure */
    stat = db_driver_close_database();

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* clear the driver state */
    db__mark_database_closed();
    db__init_driver_state();

    /* no results */
    return DB_OK;
}
