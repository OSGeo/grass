#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_grant_on_table(dbString *tableName G_UNUSED, int priv G_UNUSED,
                              int to G_UNUSED)
{
    return DB_OK;
}
