#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_drop_column(dbString *tableName UNUSED,
                           dbString *columnName UNUSED)
{
    db_procedure_not_implemented("db_drop_column");
    return DB_FAILED;
}
