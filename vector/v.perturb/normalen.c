/*
 * Copyright (C) 1994. James Darrell McCauley.  (darrell@mccauley-usa.com)
 *                                              http://mccauley-usa.com/
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */

#include<stdio.h>
#include<math.h>
#include"zufall.h"

  /* Box-Muller method for Gaussian random numbers */

int normalen(int n, double *x)
{
    static int buffsz = 1024;
    extern struct klotz1 klotz1_1;
    int left, i, nn, ptr;

    /* Parameter adjustments */
    --x;

    nn = n;
    if (nn <= 0)
	return 0;

    if (klotz1_1.first == 0) {
	normal00();
	klotz1_1.first = 1;
    }
    ptr = 0;

  L1:
    left = buffsz - klotz1_1.xptr;
    if (nn < left) {
	for (i = 1; i <= nn; ++i)
	    x[i + ptr] = klotz1_1.xbuff[klotz1_1.xptr + i - 1];
	klotz1_1.xptr += nn;
	return 0;
    }
    else {
	for (i = 1; i <= left; ++i)
	    x[i + ptr] = klotz1_1.xbuff[klotz1_1.xptr + i - 1];
	klotz1_1.xptr = 0;
	ptr += left;
	nn -= left;
	normal00();
	goto L1;
    }
}				/* normalen */
