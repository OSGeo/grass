/****************************************************************************
 *
 * MODULE:       r.coin
 *
 * AUTHOR(S):    Michael O'Shea - CERL
 *               Michael Shapiro - CERL
 *
 * PURPOSE:      Calculates the coincidence of two raster map layers.
 *
 * SPDX-FileCopyrightText: 2006 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *

***************************************************************************/

#include <stdio.h>
#include <string.h>

int format_double(double v, char *buf, int n)
{
    char fmt[15];
    int k;

    snprintf(fmt, sizeof(fmt), "%%%d.2lf", n);
    snprintf(buf, sizeof(20), fmt, v);

    for (k = n; (ssize_t)strlen(buf) > n; k--) {
        snprintf(fmt, sizeof(fmt), "%%%d.%dg", n, k);
        snprintf(buf, sizeof(20), fmt, v);
    }

    return 0;
}
