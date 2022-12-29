/*
 * Copyright (C) 1994. James Darrell McCauley.  (darrell@mccauley-usa.com)
 *                                              http://mccauley-usa.com/
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */

#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "zufall.h"

int normalrs(double *svbox)
{
    int i, k;
    extern struct klotz1 klotz1_1;

    /*
     * restores common blocks klotz0, klotz1 containing buffers and
     * pointers. IMPORTANT: svbox must be dimensioned at least 1634 in
     * driver. The entire contents of klotz0 and klotz1 must be restored.
     */

    /* restore zufall blocks klotz0 and klotz1 */

    zufallrs(svbox);
    klotz1_1.first = (int)svbox[608];
    if (klotz1_1.first == 0)
	G_warning(_("normalsv: restoration of uninitialized block"));
    klotz1_1.xptr = (int)svbox[609];
    k = 609;
    for (i = 0; i < 1024; ++i)
	klotz1_1.xbuff[i] = svbox[i + k];

    return 0;
}				/* normalrs */
