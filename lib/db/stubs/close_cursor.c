#include <grass/dbmi.h>
#include <grass/dbstubs.h>

/*!
   This function calls db_procedure_not_implemented().
 */

<<<<<<< HEAD
int db__driver_close_cursor(dbCursor *cursor UNUSED)
=======
int db__driver_close_cursor(dbCursor *cursor)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    db_procedure_not_implemented("db_close_cursor");
    return DB_FAILED;
}
