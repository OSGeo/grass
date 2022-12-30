#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_execute_immediate(dbString *SQLstatement UNUSED)
=======
int db__driver_execute_immediate(dbString *SQLstatement)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    db_procedure_not_implemented("db_execute_immediate");
    return DB_FAILED;
}
