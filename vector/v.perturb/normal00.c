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

int normal00(void)
{
    int i;
    double twopi, r1, r2, t1, t2;
    extern struct klotz1 klotz1_1;

    twopi = 6.2831853071795862;
    zufall(1024, klotz1_1.xbuff);

    /* VOCL LOOP, TEMP(r1,r2,t1,t2), NOVREC(xbuff) */

    for (i = 1; i <= 1024; i += 2) {
	r1 = twopi * klotz1_1.xbuff[i - 1];
	t1 = cos(r1);
	t2 = sin(r1);
	r2 = sqrt(log((double)1. - klotz1_1.xbuff[i]) * (double)-2.);
	klotz1_1.xbuff[i - 1] = t1 * r2;
	klotz1_1.xbuff[i] = t2 * r2;
    }
    return 0;
}				/* normal00 */
