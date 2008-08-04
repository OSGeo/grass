#include <grass/dbmi.h>
#include "odbc.h"
#include "globals.h"

int db__driver_init(argc, argv)
     char *argv[];
{
    return DB_OK;
}

int db__driver_finish()
{
    return DB_OK;
}
