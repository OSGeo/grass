#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_drop_column(dbString *tableName G_UNUSED,
                           dbString *columnName G_UNUSED)
{
    db_procedure_not_implemented("db_drop_column");
    return DB_FAILED;
}
