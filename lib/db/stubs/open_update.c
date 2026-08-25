#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_open_update_cursor(dbString *name G_UNUSED,
                                  dbString *select G_UNUSED,
                                  dbCursor *cursor G_UNUSED, int mode G_UNUSED)
{
    db_procedure_not_implemented("db_open_update_cursor");
    return DB_FAILED;
}
