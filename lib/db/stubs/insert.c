#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_insert(dbCursor *cursor UNUSED)
{
    db_procedure_not_implemented("db_insert");
    return DB_FAILED;
}
