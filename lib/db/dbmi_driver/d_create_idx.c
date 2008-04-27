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
db_d_create_index()
{
    dbIndex index;
    int stat;

/* get the arg(s) */
    db_init_index (&index);
    DB_RECV_INDEX(&index);
    
/* call the procedure */
    stat = db_driver_create_index (&index);

/* send the return code */
    if (stat != DB_OK)
    {
	db_free_index (&index);
	DB_SEND_FAILURE();
	return DB_OK;
    }
    DB_SEND_SUCCESS();

/* send results */
    DB_SEND_STRING(&index.indexName);
    db_free_index (&index);
    return DB_OK;
}
