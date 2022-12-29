/*!
  \file lib/db/dbmi_base/xdrtoken.c
  
  \brief DBMI Library (base) - external data representation (token)
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek, Brad Douglas, Markus Neteler
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/dbmi.h>

/*!
  \brief Send token

  \param token

  \return
*/
int db__send_token(dbToken * token)
{
    return db__send_int(*token);
}

/*!
  \brief Receive token

  \param token

  \return
*/
int db__recv_token(dbToken * token)
{
    return db__recv_int(token);
}
