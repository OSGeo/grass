#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_create_table(dbTable *table UNUSED)
=======
int db__driver_create_table(dbTable *table)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    db_procedure_not_implemented("db_create_table");
    return DB_FAILED;
}
