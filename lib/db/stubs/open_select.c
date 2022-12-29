#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int
db__driver_open_select_cursor(dbString * select, dbCursor * cursor, int mode)
{
    db_procedure_not_implemented("db_open_select_cursor");
    return DB_FAILED;
}
