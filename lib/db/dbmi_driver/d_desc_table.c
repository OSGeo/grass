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
db_d_describe_table()
{
    dbTable *table;
    dbString name;
    int stat;

    db_init_string (&name);

/* get the arg(s) */
    DB_RECV_STRING(&name);

/* call the procedure */
    stat = db_driver_describe_table (&name, &table);

/* send the return code */
    if (stat != DB_OK)
    {
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

/* results */
    db_set_table_name (table, db_get_string(&name));
    DB_SEND_TABLE_DEFINITION (table);

    db_free_string (&name);
    db_free_table (table);
    return DB_OK;
}
