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
int db_d_execute_immediate()
{
    int stat;
    dbString SQLstatement;

    /* get the arg(s) */
    db_init_string(&SQLstatement);
    DB_RECV_STRING(&SQLstatement);

    /* call the procedure */
    stat = db_driver_execute_immediate(&SQLstatement);
    db_free_string(&SQLstatement);

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_d_begin_transaction()
{
    int stat;

    /* call the procedure */
    stat = db_driver_begin_transaction();

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_d_commit_transaction()
{
    int stat;

    /* call the procedure */
    stat = db_driver_commit_transaction();

    /* send the return code */
    if (stat != DB_OK) {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

    /* no results */
    return DB_OK;
}
