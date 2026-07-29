#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_describe_table(dbString *name G_UNUSED, dbTable **table G_UNUSED)
{
    db_procedure_not_implemented("db_describe_table");
    return DB_FAILED;
}
