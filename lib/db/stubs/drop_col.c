#include <grass/dbmi.h>

int db__driver_drop_column(dbString * tableName, dbString * columnName)
{
    db_procedure_not_implemented("db_drop_column");
    return DB_FAILED;
}
