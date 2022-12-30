#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_list_databases(dbString *path UNUSED, int npaths UNUSED,
                              dbHandle **handles UNUSED, int *num UNUSED)
=======
int db__driver_list_databases(dbString *path, int npaths, dbHandle **handles,
                              int *num)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    db_procedure_not_implemented("db_list_databases");
    return DB_FAILED;
}
