#include<stdio.h>
#include<math.h>
#include"zufall.h"

/*
 * saves common blocks klotz0, containing seeds and pointer to position in
 * seed block. IMPORTANT: svblk must be dimensioned at least 608 in
 * driver. The entire contents of klotz0 (pointer in buff, and buff) must
 * be saved.
 */

int zufallsv(double *svblk)
{
    int i;
    extern struct klotz0 klotz0_1;

    svblk[0] = (double)klotz0_1.ptr;
    for (i = 0; i < 607; ++i)
	svblk[i + 1] = klotz0_1.buff[i];
    return 0;
}				/* zufallsv */
