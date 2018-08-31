/*!
  \file lib/db/dbmi_base/xdrprocedure.c
  
  \brief DBMI Library (base) - external data representation (procedure)
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek, Brad Douglas, Markus Neteler
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include "xdr.h"
#include "macros.h"

/*!
  \brief ? (client only)

  \param procnum

  \return
*/
int db__start_procedure_call(int procnum)
{
    int reply;

    DB_SEND_INT(procnum);
    DB_RECV_INT(&reply);
    if (reply != procnum) {
	if (reply == 0) {
	    db_noproc_error(procnum);
	}
	else {
	    db_protocol_error();
	}
	return DB_PROTOCOL_ERR;
    }

    return DB_OK;
}

/*!
  \brief ? (driver only)

  \param n

  \return DB_OK  ok
  \return DB_EOF eof from client
*/
int db__recv_procnum(int *n)
{
    int stat = DB_OK;

    if (!db__recv(n, sizeof(*n)))
	stat = DB_EOF;

    return stat;
}

/*!
  \brief ?

  \param n

  \return
*/
int db__send_procedure_ok(int n)
{
    return db__send_int(n);
}

/*!
  \brief ?

  \param n

  \return
*/
int db__send_procedure_not_implemented(int n)
{
    return db__send_int(n ? 0 : -1);
}
