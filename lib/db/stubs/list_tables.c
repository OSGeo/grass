#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_list_tables(dbString **names UNUSED, int *count UNUSED,
                           int system UNUSED)
{
    db_procedure_not_implemented("db_list_tables");
    return DB_FAILED;
}
