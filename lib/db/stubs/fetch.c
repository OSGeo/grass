#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_fetch(dbCursor *cursor, int position, int *more)
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
int db__driver_fetch(dbCursor *cursor UNUSED, int position UNUSED,
                     int *more UNUSED)
=======
int db__driver_fetch(dbCursor *cursor, int position, int *more)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_fetch(dbCursor *cursor, int position, int *more)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    db_procedure_not_implemented("db_fetch");
    return DB_FAILED;
}
