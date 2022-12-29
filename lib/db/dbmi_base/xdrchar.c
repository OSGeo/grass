/*!
  \file lib/db/dbmi_base/xdrchar.c
  
  \brief DBMI Library (base) - external data representation (char)
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek, Brad Douglas, Markus Neteler
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include "xdr.h"

/*!
  \brief ?
  
  \param d

  \return
*/
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

/*!
  \brief ?

  \param d

  \return
*/
int db__recv_char(char *d)
{
    int stat = DB_OK;

    if (!db__recv(d, sizeof(*d)))
	stat = DB_PROTOCOL_ERR;

    if (stat == DB_PROTOCOL_ERR)
	db_protocol_error();

    return stat;
}
