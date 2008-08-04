#include <grass/dbmi.h>
#include "macros.h"


int db__send_handle(dbHandle * handle)
{
    DB_SEND_STRING(&handle->dbName);
    DB_SEND_STRING(&handle->dbSchema);

    return DB_OK;
}

int db__recv_handle(dbHandle * handle)
{
    DB_RECV_STRING(&handle->dbName);
    DB_RECV_STRING(&handle->dbSchema);

    return DB_OK;
}
