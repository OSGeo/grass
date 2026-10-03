#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_fetch(dbCursor *cursor G_UNUSED, int position G_UNUSED,
                     int *more G_UNUSED)
{
    db_procedure_not_implemented("db_fetch");
    return DB_FAILED;
}
