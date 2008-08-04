#include "xdr.h"


int db__send_float(float d)
{
    int stat = DB_OK;

    if (!db__send(&d, sizeof(d)))
	stat = DB_PROTOCOL_ERR;

    if (stat == DB_PROTOCOL_ERR)
	db_protocol_error();

    return stat;
}

int db__recv_float(float *d)
{
    int stat = DB_OK;

    if (!db__recv(d, sizeof(*d)))
	stat = DB_PROTOCOL_ERR;

    if (stat == DB_PROTOCOL_ERR)
	db_protocol_error();

    return stat;
}


int db__send_float_array(const float *x, int n)
{
    int stat = DB_OK;

    if (!db__send(&n, sizeof(n)))
	stat = DB_PROTOCOL_ERR;

    if (!db__send(x, n * sizeof(*x)))
	stat = DB_PROTOCOL_ERR;

    if (stat == DB_PROTOCOL_ERR)
	db_protocol_error();

    return stat;
}

/* returns an allocated array of floats */
/* caller is responsible for free() */
int db__recv_float_array(float **x, int *n)
{
    int stat = DB_OK;
    int count = 0;
    float *a = NULL;

    if (!db__recv(&count, sizeof(count)))
	stat = DB_PROTOCOL_ERR;

    *n = count;

    *x = a = (float *)db_calloc(count, sizeof(*a));

    if (!db__recv(a, count * sizeof(*a)))
	stat = DB_PROTOCOL_ERR;

    if (stat == DB_PROTOCOL_ERR)
	db_protocol_error();

    return stat;
}
