#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_get_num_rows(dbCursor * cursor)
{
    db_procedure_not_implemented("db_get_num_rows");
    return -1;
}
