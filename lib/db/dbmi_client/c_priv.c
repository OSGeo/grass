#include <grass/dbmi.h>
#include "macros.h"

/*!
   \brief Grant privileges 
   \return 
   \param priv privileges: DB_PRIV_SELECT
   \param to grant to : DB_GROUP | DB_PUBLIC
 */
int
db_grant_on_table(dbDriver * driver, const char *tableName, int priv, int to)
{
    int ret_code;
    dbString name;

    db_init_string(&name);
    db_set_string(&name, tableName);

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_GRANT_ON_TABLE);

    /* send the argument(s) to the procedure */
    DB_SEND_STRING(&name);
    DB_SEND_INT(priv);
    DB_SEND_INT(to);

    db_free_string(&name);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* no results */
    return DB_OK;
}
