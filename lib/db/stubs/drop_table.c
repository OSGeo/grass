#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_drop_table(dbString *name G_UNUSED)
{
    db_procedure_not_implemented("db_drop_table");
    return DB_FAILED;
}
