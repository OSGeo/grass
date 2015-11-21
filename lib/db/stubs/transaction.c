#include <grass/dbmi.h>
#include <grass/dbstubs.h>

/* Implemented only in some drivers */
int db__driver_begin_transaction(void)
{
    G_debug(2, "Begin transaction");
    return DB_OK;
}

/* Implemented only in some drivers */
int db__driver_commit_transaction(void)
{
    G_debug(2, "Commit transaction");
    return DB_OK;
}
