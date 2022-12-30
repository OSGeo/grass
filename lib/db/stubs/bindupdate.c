#include <grass/dbmi.h>
#include <grass/dbstubs.h>

/*!
   \fn int db__driver_bind_update (cursor)
   \brief
   \return
   \param
 */
<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_bind_update(dbCursor *cursor UNUSED)
=======
int db__driver_bind_update(dbCursor *cursor)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_bind_update(dbCursor *cursor)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
{
    db_procedure_not_implemented("db_bind_update");
    return DB_FAILED;
}
