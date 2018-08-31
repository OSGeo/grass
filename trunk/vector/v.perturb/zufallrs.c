#include<stdio.h>
#include<math.h>
#include"zufall.h"

/*
 * restores common block klotz0, containing seeds and pointer to position
 * in seed block. IMPORTANT: svblk must be dimensioned at least 608 in
 * driver. The entire contents of klotz0 must be restored.
 */

int zufallrs(double *svblk)
{
    int i;
    extern struct klotz0 klotz0_1;

    klotz0_1.ptr = (int)svblk[0];
    for (i = 0; i < 607; ++i)
	klotz0_1.buff[i] = svblk[i + 1];

    return 0;
}				/* zufallrs */
