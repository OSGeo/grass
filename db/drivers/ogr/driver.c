/*!
   \file db/drivers/driver.c

   \brief Low level OGR SQL driver

   (C) 2004-2009 by the GRASS Development Team
    SPDX-License-Identifier: GPL-2.0-or-later
\author Radim Blazek
   \author Some updates by Martin Landa <landa.martin gmail.com>
 */

#include <grass/dbmi.h>

#include "ogr_api.h"
#include "globals.h"
#include "proto.h"

/*!
   \brief Initialize driver

   \param argc number of arguments
   \param argv array of arguments

   \return DB_OK on success
 */
int db__driver_init(int argc G_UNUSED, char *argv[] G_UNUSED)
{
    init_error();
    return DB_OK;
}

/*!
   \brief Finish driver

   \return DB_OK
 */
int db__driver_finish(void)
{
    return DB_OK;
}
