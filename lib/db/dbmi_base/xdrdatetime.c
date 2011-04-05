/*!
  \file lib/db/dbmi_base/xdrdatetime.c
  
  \brief DBMI Library (base) - external data representation (datatime)
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek, Brad Douglas, Markus Neteler
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/dbmi.h>
#include "macros.h"

/*!
  \brief Send datetime

  \param t pointer to dbDateTime

  \return DB_OK
*/
int db__send_datetime(dbDateTime * t)
{
    DB_SEND_CHAR(t->current);
    if (!t->current) {
	DB_SEND_INT(t->year);
	DB_SEND_INT(t->month);
	DB_SEND_INT(t->day);
	DB_SEND_INT(t->hour);
	DB_SEND_INT(t->minute);
	DB_SEND_DOUBLE(t->seconds);
    }

    return DB_OK;
}

/*!
  \brief Receive datetime

  \param t pointer to dbDateTime

  \return DB_OK
*/
int db__recv_datetime(dbDateTime * t)
{
    DB_RECV_CHAR(&t->current);
    if (!t->current) {
	DB_RECV_INT(&t->year);
	DB_RECV_INT(&t->month);
	DB_RECV_INT(&t->day);
	DB_RECV_INT(&t->hour);
	DB_RECV_INT(&t->minute);
	DB_RECV_DOUBLE(&t->seconds);
    }

    return DB_OK;
}
