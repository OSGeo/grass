#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_grant_on_table(dbString *tableName UNUSED, int priv UNUSED,
                              int to UNUSED)
{
    return DB_OK;
}
