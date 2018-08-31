/*!
  \file lib/db/dbmi_base/xdrvalue.c
  
  \brief DBMI Library (base) - external data representation (value)
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek, Brad Douglas, Markus Neteler
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "macros.h"

/*!
  \brief Send value

  \param value
  \param Ctype

  \return
*/
int db__send_value(dbValue * value, int Ctype)
{
    DB_SEND_CHAR(value->isNull);
    if (value->isNull)
	return DB_OK;

    switch (Ctype) {
    case DB_C_TYPE_INT:
	DB_SEND_INT(value->i);
	break;
    case DB_C_TYPE_DOUBLE:
	DB_SEND_DOUBLE(value->d);
	break;
    case DB_C_TYPE_STRING:
	DB_SEND_STRING(&value->s);
	break;
    case DB_C_TYPE_DATETIME:
	DB_SEND_DATETIME(&value->t);
	break;
    default:
	db_error("send data: invalid C-type");
	return DB_FAILED;
    }
    return DB_OK;
}

/*!
  \brief Receive value

  \param value
  \param Ctype

  \return
*/
int db__recv_value(dbValue * value, int Ctype)
{
    DB_RECV_CHAR(&value->isNull);
    if (value->isNull)
	return DB_OK;

    switch (Ctype) {
    case DB_C_TYPE_INT:
	DB_RECV_INT(&value->i);
	break;
    case DB_C_TYPE_DOUBLE:
	DB_RECV_DOUBLE(&value->d);
	break;
    case DB_C_TYPE_STRING:
	DB_RECV_STRING(&value->s);
	break;
    case DB_C_TYPE_DATETIME:
	DB_RECV_DATETIME(&value->t);
	break;
    default:
	db_error(_("send data: invalid C-type"));
	return DB_FAILED;
    }
    return DB_OK;
}
