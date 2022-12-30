#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_grant_on_table(dbString *tableName UNUSED, int priv UNUSED,
                              int to UNUSED)
=======
int db__driver_grant_on_table(dbString *tableName, int priv, int to)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    return DB_OK;
}
