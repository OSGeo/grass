#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_insert(dbCursor *cursor)
=======
<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_insert(dbCursor *cursor UNUSED)
=======
int db__driver_insert(dbCursor *cursor)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_insert(dbCursor *cursor)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
{
    db_procedure_not_implemented("db_insert");
    return DB_FAILED;
}
