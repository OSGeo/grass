#include "xdr.h"


int db__send_char(int d)
{
    int stat = DB_OK;
    char c = (char)d;

    if (!db__send(&c, sizeof(c)))
	stat = DB_PROTOCOL_ERR;

    if (stat == DB_PROTOCOL_ERR)
	db_protocol_error();

    return stat;
}


int db__recv_char(char *d)
{
    int stat = DB_OK;

    if (!db__recv(d, sizeof(*d)))
	stat = DB_PROTOCOL_ERR;

    if (stat == DB_PROTOCOL_ERR)
	db_protocol_error();

    return stat;
}
