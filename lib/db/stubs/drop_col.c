#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_drop_column(dbString *tableName, dbString *columnName)
=======
<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_drop_column(dbString *tableName UNUSED,
                           dbString *columnName UNUSED)
=======
int db__driver_drop_column(dbString *tableName, dbString *columnName)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_drop_column(dbString *tableName, dbString *columnName)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
{
    db_procedure_not_implemented("db_drop_column");
    return DB_FAILED;
}
