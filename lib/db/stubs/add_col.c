#include <grass/dbmi.h>
#include <grass/dbstubs.h>

/*!
   This function calls db_procedure_not_implemented().
 */
int db__driver_add_column(dbString *tableName G_UNUSED,
                          dbColumn *column G_UNUSED)
{
    db_procedure_not_implemented("db_add_column");
    return DB_FAILED;
}
