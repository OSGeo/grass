/*
 * Copyright (C) 1994. James Darrell McCauley.  (darrell@mccauley-usa.com)
 *                                              http://mccauley-usa.com/
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */

#include <stdio.h>
#include <grass/gis.h>
#include "zufall.h"

int myrng(double *numbers, int n,
	  int (*rng) (int, double *), double p1, double p2)
{
    int i;

    rng(n, numbers);


    if (rng == zufall)		/* uniform */
	for (i = 0; i < n; ++i)
	    numbers[i] -= 0.5, numbers[i] *= 2 * p1;
    else if (rng == normalen)	/* gaussian */
	/* is this how to do transformation? */
	for (i = 0; i < n; ++i)
	    numbers[i] *= p2, numbers[i] += p1;
    
    return 0;
}
