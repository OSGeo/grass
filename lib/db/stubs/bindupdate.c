#include <grass/dbmi.h>
#include <grass/dbstubs.h>

/*!
   \fn int db__driver_bind_update (cursor)
   \brief 
   \return 
   \param 
 */
int db__driver_bind_update(dbCursor * cursor)
{
    db_procedure_not_implemented("db_bind_update");
    return DB_FAILED;
}
