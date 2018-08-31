/*!
  \file lib/db/dbmi_base/xdrstring.c
  
  \brief DBMI Library (base) - external data representation (string)
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek, Brad Douglas, Markus Neteler
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <string.h>
#include "xdr.h"

/*!
  \brief Send string array

  \param a
  \param count

  \return
*/
int db__send_string_array(dbString * a, int count)
{
    int i;
    int stat;

    stat = db__send_int(count);
    for (i = 0; stat == DB_OK && i < count; i++)
	stat = db__send_string(&a[i]);

    return stat;
}

/*!
  \brief Receive string array

  \param a
  \param n

  \return
*/
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

/*!
  \brief Send string

  \param x

  \return
*/
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

/*!
  \brief Reads a string from transport
 
  Note: caller MUST initialize x by calling db_init_string()

  \param x

  \return DB_OK, DB_MEMORY_ERR, or DB_PROTOCOL_ERR
  \return NULL if error
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

/*!
  \brief Send C string

  \param s

  \return
*/
int db__send_Cstring(const char *s)
{
    dbString x;

    db_init_string(&x);
    db_set_string_no_copy(&x, (char *)s);

    return db__send_string(&x);
}
