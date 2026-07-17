/*****************************************************************************
 *
 * MODULE:       DBF driver
 *
 * AUTHOR(S):    Radim Blazek
 *
 * PURPOSE:      Simple driver for reading and writing dbf files
 *
 * SPDX-FileCopyrightText: 2000 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include "globals.h"

/* save string to value */
int save_string(VALUE *val, char *c)
{
    int len;

    len = strlen(c) + 1;
    val->c = (char *)G_realloc(val->c, len);

    strcpy(val->c, c);

    return (1);
}
