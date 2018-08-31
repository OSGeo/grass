#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_delete_database(dbHandle * handle)
{
    db_procedure_not_implemented("db_delete_database");
    return DB_FAILED;
}
