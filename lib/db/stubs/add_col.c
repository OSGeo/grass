#include <grass/dbmi.h>
#include <grass/dbstubs.h>

/*!
   This function calls db_procedure_not_implemented().
 */
<<<<<<< HEAD
int db__driver_add_column(dbString *tableName, dbColumn *column)
=======
<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_add_column(dbString *tableName UNUSED, dbColumn *column UNUSED)
=======
int db__driver_add_column(dbString *tableName, dbColumn *column)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_add_column(dbString *tableName, dbColumn *column)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
{
    db_procedure_not_implemented("db_add_column");
    return DB_FAILED;
}
