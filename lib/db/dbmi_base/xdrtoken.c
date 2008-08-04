#include <grass/dbmi.h>


int db__send_token(dbToken * token)
{
    return db__send_int(*token);
}

int db__recv_token(dbToken * token)
{
    return db__recv_int(token);
}
