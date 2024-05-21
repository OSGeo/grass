#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_open_select_cursor(dbString *select, dbCursor *cursor, int mode)
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
int db__driver_open_select_cursor(dbString *select UNUSED,
                                  dbCursor *cursor UNUSED, int mode UNUSED)
=======
int db__driver_open_select_cursor(dbString *select, dbCursor *cursor, int mode)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_open_select_cursor(dbString *select, dbCursor *cursor, int mode)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    db_procedure_not_implemented("db_open_select_cursor");
    return DB_FAILED;
}
