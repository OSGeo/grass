/*
 * Copyright (C) 1993-1994. James Darrell McCauley. (darrell@mccauley-usa.com)
 *                                http://mccauley-usa.com/
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */

#include <math.h>
#include <grass/gis.h>

int make_histo(int **p, int np, int nsites)
{
    int i, j;


    /* minimum number of sites per partition */
    j = (int)floor((double)nsites / np);

    *p = (int *)G_malloc(np * sizeof(int));

    for (i = 0; i < np; ++i)
	(*p)[i] = j;
    i = np * j;
    j = 0;
    while (i++ < nsites)
	(*p)[j++]++;

    /* return max number of sites per partition */
    return (int)ceil((double)nsites / np);
}
