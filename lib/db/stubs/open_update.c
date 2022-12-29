#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int
db__driver_open_update_cursor(dbString * name, dbString * select,
			      dbCursor * cursor, int mode)
{
    db_procedure_not_implemented("db_open_update_cursor");
    return DB_FAILED;
}
