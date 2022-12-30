#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_find_database(dbHandle *handle UNUSED, int *found UNUSED)
=======
int db__driver_find_database(dbHandle *handle, int *found)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_find_database(dbHandle *handle, int *found)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
{
    db_procedure_not_implemented("db_find_database");
    return DB_FAILED;
}
