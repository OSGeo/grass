#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_list_indexes(dbString *tableName, dbIndex **indexes, int *count)
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
int db__driver_list_indexes(dbString *tableName UNUSED,
                            dbIndex **indexes UNUSED, int *count UNUSED)
=======
int db__driver_list_indexes(dbString *tableName, dbIndex **indexes, int *count)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_list_indexes(dbString *tableName, dbIndex **indexes, int *count)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    db_procedure_not_implemented("db_list_indexes");
    return DB_FAILED;
}
