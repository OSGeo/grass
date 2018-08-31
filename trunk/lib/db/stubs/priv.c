#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_grant_on_table(dbString * tableName, int priv, int to)
{
    return DB_OK;
}
