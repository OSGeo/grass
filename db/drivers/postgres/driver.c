#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"

int db__driver_init(int argc, char *argv[])
{
    init_error();
    return DB_OK;
}

int db__driver_finish()
{
    return DB_OK;
}
