#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_list_tables(dbString **names G_UNUSED, int *count G_UNUSED,
                           int system G_UNUSED)
{
    db_procedure_not_implemented("db_list_tables");
    return DB_FAILED;
}
