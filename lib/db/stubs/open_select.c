#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_open_select_cursor(dbString *select UNUSED,
                                  dbCursor *cursor UNUSED, int mode UNUSED)
=======
int db__driver_open_select_cursor(dbString *select, dbCursor *cursor, int mode)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    db_procedure_not_implemented("db_open_select_cursor");
    return DB_FAILED;
}
