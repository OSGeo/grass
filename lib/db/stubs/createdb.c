#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_create_database(dbHandle * handle)
{
    db_procedure_not_implemented("db_create_database");
    return DB_FAILED;
}
