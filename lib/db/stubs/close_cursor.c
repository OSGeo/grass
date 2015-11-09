#include <grass/dbmi.h>
#include <grass/dbstubs.h>

/*!
   This function calls db_procedure_not_implemented().
 */

int db__driver_close_cursor(dbCursor * cursor)
{
    db_procedure_not_implemented("db_close_cursor");
    return DB_FAILED;
}
