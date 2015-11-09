#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_open_database(dbHandle * handle)
{
    db_procedure_not_implemented("db_open_database");
    return DB_FAILED;
}
