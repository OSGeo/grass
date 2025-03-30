#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_list_databases(dbString *path UNUSED, int npaths UNUSED,
                              dbHandle **handles UNUSED, int *num UNUSED)
{
    db_procedure_not_implemented("db_list_databases");
    return DB_FAILED;
}
