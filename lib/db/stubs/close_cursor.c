#include <grass/dbmi.h>

/*!
   \fn int db__driver_close_cursor (dbCursor *cursor)
   \brief 
   \return 
   \param 
 */

int db__driver_close_cursor(dbCursor * cursor)
{
    db_procedure_not_implemented("db_close_cursor");
    return DB_FAILED;
}
