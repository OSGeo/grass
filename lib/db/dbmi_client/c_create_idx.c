#include <string.h>
#include <grass/dbmi.h>
#include "macros.h"

/*!
 \fn int db_create_index (dbDriver *driver, dbIndex *index)
 \brief 
 \return 
 \param 
*/
int
db_create_index (dbDriver *driver, dbIndex *index)
{
    int ret_code;

/* start the procedure call */
    db__set_protocol_fds (driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_CREATE_INDEX);

/* send the arguments to the procedure */
    DB_SEND_INDEX (index);

/* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code; /* ret_code SHOULD == DB_FAILED */

/* get results */
    DB_RECV_STRING(&index->indexName);

    return DB_OK;
}

/*!
 \brief  Create unique index
 \return 
 \param 
*/
int
db_create_index2 (dbDriver *driver, const char *table_name, const char *column_name)
{
    int ret;
    dbIndex index;
    char buf[1000];
    const char *tbl;

    db_init_index ( &index );
    db_alloc_index_columns ( &index, 1 );

    tbl = strchr ( table_name, '.' );
    if ( tbl == NULL )
	tbl = table_name;
    else 
	tbl++;
    
    sprintf ( buf, "%s_%s", tbl, column_name );
    db_set_index_name ( &index, buf );

    db_set_index_table_name ( &index, table_name );
    db_set_index_column_name ( &index, 0, column_name );
    db_set_index_type_unique ( &index );

    ret = db_create_index ( driver , &index );

    db_free_index ( &index );

    return ret;
}
