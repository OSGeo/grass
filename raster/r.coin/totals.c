
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

#include "coin.h"


int row_total(int row, int with_no_data, long *count, double *area)
{
    int col;
    struct stats_table *x;

    x = table + row * ncat1;
    *count = 0;
    *area = 0.0;
    for (col = 0; col < ncat1; col++) {
	if (with_no_data || (col != no_data1)) {
	    *count += x->count;
	    *area += x->area;
	}
	x += 1;
    }

    return 0;
}

int col_total(int col, int with_no_data, long *count, double *area)
{
    int row;
    struct stats_table *x;

    x = table + col;
    *count = 0;
    *area = 0.0;
    for (row = 0; row < ncat2; row++) {
	if (with_no_data || (row != no_data2)) {
	    *count += x->count;
	    *area += x->area;
	}
	x += ncat1;
    }

    return 0;
}
