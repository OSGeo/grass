/*!
  \file lib/db/dbmi_base/xdrint.c
  
  \brief DBMI Library (base) - external data representation (integer)
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek, Brad Douglas, Markus Neteler
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include "xdr.h"

/*!
  \brief Send integer

  \param n

  \return
*/
int db__send_int(int n)
{
    int stat = DB_OK;

    if (!db__send(&n, sizeof(n)))
	stat = DB_PROTOCOL_ERR;

    if (stat == DB_PROTOCOL_ERR)
	db_protocol_error();

    return stat;
}

/*!
  \brief Receive integer

  \param n

  \return
*/
int db__recv_int(int *n)
{
    int stat = DB_OK;

    if (!db__recv(n, sizeof(*n)))
	stat = DB_PROTOCOL_ERR;

    if (stat == DB_PROTOCOL_ERR)
	db_protocol_error();

    return stat;
}

/*!
  \brief Send integer array

  \param x
  \param n

  \return
*/
int db__send_int_array(const int *x, int n)
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

/*!
  \brief Receive integer array

  Returns an allocated array of ints
  Caller is responsible for free()

  \param x
  \param n

  \return
*/
int db__recv_int_array(int **x, int *n)
{
    int stat = DB_OK;
    int count = 0;
    int *a = NULL;

    if (!db__recv(&count, sizeof(count)))
	stat = DB_PROTOCOL_ERR;

    *n = count;

    *x = a = (int *)db_calloc(count, sizeof(*a));

    if (!db__recv(a, count * sizeof(*a)))
	stat = DB_PROTOCOL_ERR;

    if (stat == DB_PROTOCOL_ERR)
	db_protocol_error();

    return stat;
}
