#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int
db__driver_list_databases(dbString * path, int npaths, dbHandle ** handles,
			  int *num)
{
    db_procedure_not_implemented("db_list_databases");
    return DB_FAILED;
}
