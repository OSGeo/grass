#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_list_tables(dbString **names UNUSED, int *count UNUSED,
                           int system UNUSED)
=======
int db__driver_list_tables(dbString **names, int *count, int system)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    db_procedure_not_implemented("db_list_tables");
    return DB_FAILED;
}
