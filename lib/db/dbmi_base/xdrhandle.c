/*!
   \file lib/db/dbmi_base/xdrhandle.c

   \brief DBMI Library (base) - external data representation (handle)

   SPDX-FileCopyrightText: 1999-2009, 2011 by the GRASS Development Team

   SPDX-License-Identifier: GPL-2.0-or-later.

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
int db__send_handle(dbHandle *handle)
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
int db__recv_handle(dbHandle *handle)
{
    DB_RECV_STRING(&handle->dbName);
    DB_RECV_STRING(&handle->dbSchema);

    return DB_OK;
}
