#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_list_databases(dbString *path G_UNUSED, int npaths G_UNUSED,
                              dbHandle **handles G_UNUSED, int *num G_UNUSED)
{
    db_procedure_not_implemented("db_list_databases");
    return DB_FAILED;
}
