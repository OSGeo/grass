#include <grass/dbmi.h>

int
db__driver_list_indexes(dbString * tableName, dbIndex ** indexes, int *count)
{
    db_procedure_not_implemented("db_list_indexes");
    return DB_FAILED;
}
