#include <grass/dbmi.h>

int db__driver_execute_immediate(dbString * SQLstatement)
{
    db_procedure_not_implemented("db_execute_immediate");
    return DB_FAILED;
}
