#include "dbmi.h"
#include "macros.h"

db_begin_work (driver, result)
    dbDriver *driver;
    int *result;
{
    int ret_code;

/* start the procedure call */
    db__set_protocol_fds (driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_BEGIN_WORK);

/* no args */

/* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code; /* ret_code SHOULD == DB_FAILED */

/*  results */
    DB_RECV_INT (result);

    return DB_OK;
}
