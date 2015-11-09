#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_close_database(void)
{
    db_procedure_not_implemented("db_close_database");
    return DB_FAILED;
}
