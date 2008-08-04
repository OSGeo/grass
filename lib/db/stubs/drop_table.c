#include <grass/dbmi.h>

int db__driver_drop_table(dbString * name)
{
    db_procedure_not_implemented("db_drop_table");
    return DB_FAILED;
}
