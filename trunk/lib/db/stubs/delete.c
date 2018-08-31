#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_delete(dbCursor * cursor)
{
    db_procedure_not_implemented("db_delete");
    return DB_FAILED;
}
