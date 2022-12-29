#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_drop_index(dbString * name)
{
    db_procedure_not_implemented("db_drop_index");
    return DB_FAILED;
}
