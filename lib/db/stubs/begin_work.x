#include "dbmi.h"

db_driver_begin_work (result)
    int *result;
{
    db_procedure_not_implemented("db_begin_work");
    return DB_FAILED;
}
