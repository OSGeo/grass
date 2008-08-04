#include <stdlib.h>
#include <grass/dbmi.h>

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_init_handle(dbHandle * handle)
{
    db_init_string(&handle->dbName);
    db_init_string(&handle->dbSchema);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_set_handle(dbHandle * handle, const char *dbName, const char *dbSchema)
{
    int stat;

    stat = db_set_string(&handle->dbName, dbName);
    if (stat != DB_OK)
	return stat;
    stat = db_set_string(&handle->dbSchema, dbSchema);
    return stat;
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
const char *db_get_handle_dbname(dbHandle * handle)
{
    return db_get_string(&handle->dbName);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
const char *db_get_handle_dbschema(dbHandle * handle)
{
    return db_get_string(&handle->dbSchema);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_free_handle(dbHandle * handle)
{
    db_free_string(&handle->dbName);
    db_free_string(&handle->dbSchema);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
void db_free_handle_array(dbHandle * handle, int count)
{
    int i;

    if (handle) {
	for (i = 0; i < count; i++)
	    db_free_handle(&handle[i]);
	free(handle);
    }
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
dbHandle *db_alloc_handle_array(int count)
{
    int i;
    dbHandle *handle;

    handle = (dbHandle *) db_calloc(count, sizeof(dbHandle));
    if (handle)
	for (i = 0; i < count; i++)
	    db_init_handle(&handle[i]);
    return handle;
}
