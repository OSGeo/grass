#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_find_database(dbHandle *handle UNUSED, int *found UNUSED)
{
    db_procedure_not_implemented("db_find_database");
    return DB_FAILED;
}
