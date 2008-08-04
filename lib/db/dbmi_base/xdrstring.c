#include <string.h>
#include "xdr.h"


int db__send_string_array(dbString * a, int count)
{
    int i;
    int stat;

    stat = db__send_int(count);
    for (i = 0; stat == DB_OK && i < count; i++)
	stat = db__send_string(&a[i]);

    return stat;
}

/* note: dbString *a; ...(...,&a...) */
int db__recv_string_array(dbString ** a, int *n)
{
    int i, count;
    int stat;
    dbString *b;

    *n = 0;
    *a = NULL;
    stat = db__recv_int(&count);
    if (stat != DB_OK)
	return stat;
    if (count < 0) {
	db_protocol_error();
	return DB_PROTOCOL_ERR;
    }

    b = db_alloc_string_array(count);
    if (b == NULL)
	return DB_MEMORY_ERR;

    for (i = 0; i < count; i++) {
	stat = db__recv_string(&b[i]);
	if (stat != DB_OK) {
	    db_free_string_array(b, count);
	    return stat;
	}
    }
    *n = count;
    *a = b;

    return DB_OK;
}

int db__send_string(dbString * x)
{
    int stat = DB_OK;
    const char *s = db_get_string(x);
    int len = s ? strlen(s) + 1 : 1;

    if (!s)
	s = "";			/* don't send a NULL string */

    if (!db__send(&len, sizeof(len)))
	stat = DB_PROTOCOL_ERR;

    if (!db__send(s, len))
	stat = DB_PROTOCOL_ERR;

    if (stat == DB_PROTOCOL_ERR)
	db_protocol_error();

    return stat;
}

/*
 * db__recv_string (dbString *x)
 *  reads a string from transport
 *
 *  returns DB_OK, DB_MEMORY_ERR, or DB_PROTOCOL_ERR
 *    x.s will be NULL if error
 *
 * NOTE: caller MUST initialize x by calling db_init_string()
 */
int db__recv_string(dbString * x)
{
    int stat = DB_OK;
    int len;
    char *s;

    if (!db__recv(&len, sizeof(len)))
	stat = DB_PROTOCOL_ERR;

    if (len <= 0)		/* len will include the null byte */
	stat = DB_PROTOCOL_ERR;

    if (db_enlarge_string(x, len) != DB_OK)
	stat = DB_PROTOCOL_ERR;

    s = db_get_string(x);

    if (!db__recv(s, len))
	stat = DB_PROTOCOL_ERR;

    if (stat == DB_PROTOCOL_ERR)
	db_protocol_error();

    return stat;
}

int db__send_Cstring(const char *s)
{
    dbString x;

    db_init_string(&x);
    db_set_string_no_copy(&x, (char *)s);

    return db__send_string(&x);
}
