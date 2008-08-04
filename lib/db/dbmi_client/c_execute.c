#include <grass/dbmi.h>
#include "macros.h"

/*!
   \fn int db_execute_immediate (dbDriver *driver, dbString *SQLstatement)
   \brief 
   \return 
   \param 
 */
int db_execute_immediate(dbDriver * driver, dbString * SQLstatement)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_EXECUTE_IMMEDIATE);

    /* send the argument(s) to the procedure */
    DB_SEND_STRING(SQLstatement);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* no results */
    return DB_OK;
}

/*!
   \fn int db_begin_transaction (dbDriver *driver)
   \brief 
   \return 
   \param 
 */
int db_begin_transaction(dbDriver * driver)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_BEGIN_TRANSACTION);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;

    /* no results */
    return DB_OK;
}

/*!
   \fn int db_commit_transaction (dbDriver *driver)
   \brief 
   \return 
   \param 
 */
int db_commit_transaction(dbDriver * driver)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_COMMIT_TRANSACTION);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;

    /* no results */
    return DB_OK;
}
