/*!
  \file lib/db/dbmi_base/handle.c
  
  \brief DBMI Library (base) - handle management
  
  (C) 1999-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
*/

#include <stdlib.h>
#include <grass/dbmi.h>

/*!
  \brief Initialize handle (i.e database/schema)

  \param handle pointer to dbHandle to be initialized
*/
void db_init_handle(dbHandle * handle)
{
    db_init_string(&handle->dbName);
    db_init_string(&handle->dbSchema);
}

/*!
  \brief Set handle (database and schema name)

  \param handle pointer to dbHandle
  \param dbName database name
  \param dbSchema schema name

  \return DB_OK on success
  \return DB_FAILED on failure
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
  \brief Get handle database name

  \param handle pointer to dbHandle

  \return pointer to string with database name
*/
const char *db_get_handle_dbname(dbHandle * handle)
{
    return db_get_string(&handle->dbName);
}

/*!
  \brief Get handle schema name

  \param handle pointer to dbHandle

  \return pointer to string with schema name
*/
const char *db_get_handle_dbschema(dbHandle * handle)
{
    return db_get_string(&handle->dbSchema);
}

/*!
  \brief Free dbHandle structure
  
  \param handle pointer to dbHandle
*/
void db_free_handle(dbHandle * handle)
{
    db_free_string(&handle->dbName);
    db_free_string(&handle->dbSchema);
}

/*!
  \brief Free array of handles

  \param handle pointer to first dbHandle in the array
  \param count number of handles in the array
*/
void db_free_handle_array(dbHandle * handle, int count)
{
    int i;

    if (handle) {
	for (i = 0; i < count; i++)
	    db_free_handle(&handle[i]);
	db_free((void *) handle);
    }
}

/*!
  \brief Allocate array of handles

  \param count number of handles in the array

  \return pointer to first dbHandle in the array
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
