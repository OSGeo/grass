#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_describe_table(dbString *name, dbTable **table)
=======
<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_describe_table(dbString *name UNUSED, dbTable **table UNUSED)
=======
int db__driver_describe_table(dbString *name, dbTable **table)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_describe_table(dbString *name, dbTable **table)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
{
    db_procedure_not_implemented("db_describe_table");
    return DB_FAILED;
}
