#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_create_database(dbHandle *handle)
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
int db__driver_create_database(dbHandle *handle UNUSED)
=======
int db__driver_create_database(dbHandle *handle)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_create_database(dbHandle *handle)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    db_procedure_not_implemented("db_create_database");
    return DB_FAILED;
}
