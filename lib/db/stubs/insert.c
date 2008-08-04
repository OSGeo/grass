#include <grass/dbmi.h>

int db__driver_insert(dbCursor * cursor)
{
    db_procedure_not_implemented("db_insert");
    return DB_FAILED;
}
