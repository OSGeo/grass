
/****************************************************************************
 *
 * MODULE:       r.coin
 *
 * AUTHOR(S):    Michael O'Shea - CERL
 *               Michael Shapiro - CERL
 *
 * PURPOSE:      Calculates the coincidence of two raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 
***************************************************************************/

#include <stdio.h>
#include <string.h>


int format_double(double v, char *buf, int n)
{
    char fmt[15];
    int k;

    sprintf(fmt, "%%%d.2lf", n);
    sprintf(buf, fmt, v);

    for (k = n; strlen(buf) > n; k--) {
	sprintf(fmt, "%%%d.%dg", n, k);
	sprintf(buf, fmt, v);
    }

    return 0;
}
