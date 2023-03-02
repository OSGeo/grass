/*!
   \file db/drivers/error.c

   \brief Low level OGR SQL driver

   (C) 2004-2009 by the GRASS Development Team
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
   \author Some updates by Martin Landa <landa.martin gmail.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <grass/gis.h>
#include <grass/dbmi.h>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <ogr_api.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "globals.h"
#include "proto.h"

/* init error message */
void init_error(void)
{
    db_d_init_error("OGR");
}
