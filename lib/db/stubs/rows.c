#include <grass/dbmi.h>
#include <grass/dbstubs.h>

<<<<<<< HEAD
int db__driver_get_num_rows(dbCursor *cursor UNUSED)
=======
int db__driver_get_num_rows(dbCursor *cursor)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    db_procedure_not_implemented("db_get_num_rows");
    return -1;
}
