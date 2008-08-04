#include <grass/dbmi.h>

int db__driver_fetch(dbCursor * cursor, int position, int *more)
{
    db_procedure_not_implemented("db_fetch");
    return DB_FAILED;
}
