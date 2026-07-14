/****************************************************************************
 *
 * MODULE:       r.cross
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Creates a cross product of the category values from
 *               multiple raster map layers.
 *
 * SPDX-FileCopyrightText: 2006 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 ***************************************************************************/

#include <string.h>
#include <grass/raster.h>
#include "glob.h"

static char *get_label(CELL, struct Categories *);

int set_cat(CELL result, CELL *cat, struct Categories *pcats)
{
    int i, n;
    static char *buf = NULL;
    static int len = 0;
    char *lbl;

    n = 0;
    for (i = 0; i < nfiles; i++) {
        lbl = get_label(cat[i], &labels[i]);
        n += strlen(lbl) + 2;
    }
    if (len < n)
        buf = G_realloc(buf, len = n);

    *buf = 0;
    for (i = 0; i < nfiles; i++) {
        if (i)
            strcat(buf, "; ");
        lbl = get_label(cat[i], &labels[i]);
        strcat(buf, lbl);
    }
    Rast_set_c_cat(&result, &result, buf, pcats);
    return 0;
}

static char *get_label(CELL cat, struct Categories *lbls)
{
    char *lbl;
    static char temp[256];

    if (Rast_is_c_null_value(&cat)) {
        snprintf(lbl = temp, sizeof(temp), "NULL");
    }
    else {
        lbl = Rast_get_c_cat(&cat, lbls);
        if (*lbl == 0)
            snprintf(lbl = temp, sizeof(temp), "category %d", cat);
    }

    return lbl;
}
