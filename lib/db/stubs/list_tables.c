#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_list_tables(dbString **names, int *count, int system)
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
int db__driver_list_tables(dbString **names UNUSED, int *count UNUSED,
                           int system UNUSED)
=======
int db__driver_list_tables(dbString **names, int *count, int system)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_list_tables(dbString **names, int *count, int system)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    db_procedure_not_implemented("db_list_tables");
    return DB_FAILED;
}
