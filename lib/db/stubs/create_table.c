#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_create_table(dbTable * table)
{
    db_procedure_not_implemented("db_create_table");
    return DB_FAILED;
}
