#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_delete_database(dbHandle *handle UNUSED)
=======
int db__driver_delete_database(dbHandle *handle)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    db_procedure_not_implemented("db_delete_database");
    return DB_FAILED;
}
