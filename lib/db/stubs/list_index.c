#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_list_indexes(dbString *tableName G_UNUSED,
                            dbIndex **indexes G_UNUSED, int *count G_UNUSED)
{
    db_procedure_not_implemented("db_list_indexes");
    return DB_FAILED;
}
