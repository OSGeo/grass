#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_drop_table(dbString *name)
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
int db__driver_drop_table(dbString *name UNUSED)
=======
int db__driver_drop_table(dbString *name)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_drop_table(dbString *name)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    db_procedure_not_implemented("db_drop_table");
    return DB_FAILED;
}
