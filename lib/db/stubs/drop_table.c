#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_drop_table(dbString *name UNUSED)
=======
int db__driver_drop_table(dbString *name)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    db_procedure_not_implemented("db_drop_table");
    return DB_FAILED;
}
