/*!
   \file db/driver/postgres/error.c

   \brief DBMI - Low Level PostgreSQL database driver - report errors

    SPDX-License-Identifier: GPL-2.0-or-later
\author Radim Blazek
 */

#include <stdio.h>
#include <grass/gis.h>
#include <grass/dbmi.h>

#include "globals.h"

/* init error message */
void init_error(void)
{
    db_d_init_error("PostgreSQL");
}
