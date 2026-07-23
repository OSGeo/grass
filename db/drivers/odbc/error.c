/**
 * \file error.c
 *
 * \brief Low level driver error reporting function.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
*
 * \author Radim Blazek
 *
 * \date 2000-2007
 */

#include <stdio.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include "odbc.h"
#include "globals.h"

/* init error message */
void init_error(void)
{
    db_d_init_error("ODBC");
}
