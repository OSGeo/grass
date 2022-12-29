/*!
  \file lib/db/dbmi_base/xdrhandle.c
  
  \brief DBMI Library (base) - external data representation (handle)
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek, Brad Douglas, Markus Neteler
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/dbmi.h>
#include "macros.h"

/*!
  \brief Send handle

  \param handle

  \return
*/
int db__send_handle(dbHandle * handle)
{
    DB_SEND_STRING(&handle->dbName);
    DB_SEND_STRING(&handle->dbSchema);

    return DB_OK;
}

/*!
  \brief Receive handle

  \param handle

  \return
*/
int db__recv_handle(dbHandle * handle)
{
    DB_RECV_STRING(&handle->dbName);
    DB_RECV_STRING(&handle->dbSchema);

    return DB_OK;
}
