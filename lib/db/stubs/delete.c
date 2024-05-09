#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_delete(dbCursor *cursor)
=======
<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_delete(dbCursor *cursor UNUSED)
=======
int db__driver_delete(dbCursor *cursor)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_delete(dbCursor *cursor)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
{
    db_procedure_not_implemented("db_delete");
    return DB_FAILED;
}
