#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_open_insert_cursor(dbCursor * cursor)
{
    db_procedure_not_implemented("db_open_insert_cursor");
    return DB_FAILED;
}
