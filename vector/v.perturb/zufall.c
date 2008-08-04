#include<stdio.h>
#include<math.h>
#include"zufall.h"

/*-
 * portable lagged Fibonacci series uniform random number generator with
 * "lags" -273 und -607:
 * 
 *        t    = u(i-273)+buff(i-607)  (floating pt.) 
 *        u(i) = t - float(int(t)) 
 *
 * W.P. Petersen, IPS, ETH Zuerich, 19 Mar. 92 
 */

struct klotz0 klotz0_1;
struct klotz1 klotz1_1;

int zufall(int n, double *a)
{
    static int buffsz = 607;
    int left, aptr, bptr, aptr0, i, k, q, nn, vl, qq, k273, k607;
    double t;
    extern struct klotz0 klotz0_1;

    --a;
    aptr = 0;
    nn = n;

  L1:
    if (nn <= 0)
	return 0;

    q = (nn - 1) / 607;		/* factor nn = q*607 + r */
    left = buffsz - klotz0_1.ptr;

    if (q <= 1) {		/* only one or fewer full segments */
	if (nn < left) {
	    for (i = 1; i <= nn; ++i)
		a[i + aptr] = klotz0_1.buff[klotz0_1.ptr + i - 1];
	    klotz0_1.ptr += nn;
	    return 0;
	}
	else {
	    for (i = 1; i <= left; ++i)
		a[i + aptr] = klotz0_1.buff[klotz0_1.ptr + i - 1];
	    klotz0_1.ptr = 0;
	    aptr += left;
	    nn -= left;
	    /* buff -> buff case */
	    vl = 273;
	    k273 = 334;
	    k607 = 0;
	    for (k = 1; k <= 3; ++k) {
		/* dir$ ivdep */
		/* vdir nodep */
		/* VOCL LOOP, TEMP(t), NOVREC(buff) */
		for (i = 1; i <= vl; ++i) {
		    t = klotz0_1.buff[k273 + i - 1] + klotz0_1.buff[k607 + i -
								    1];
		    klotz0_1.buff[k607 + i - 1] = t - (float)((int)t);
		}
		k607 += vl;
		k273 += vl;
		vl = 167;
		if (k == 1) {
		    k273 = 0;
		}
	    }
	    goto L1;
	}
    }
    else {
	/* more than 1 full segment */
	for (i = 1; i <= left; ++i)
	    a[i + aptr] = klotz0_1.buff[klotz0_1.ptr + i - 1];
	nn -= left;
	klotz0_1.ptr = 0;
	aptr += left;

	/* buff -> a(aptr0) */

	vl = 273;
	k273 = 334;
	k607 = 0;
	for (k = 1; k <= 3; ++k) {
	    if (k == 1) {
		/* VOCL LOOP, TEMP(t) */
		for (i = 1; i <= vl; ++i) {
		    t = klotz0_1.buff[k273 + i - 1] + klotz0_1.buff[k607 + i -
								    1];
		    a[aptr + i] = t - (float)((int)t);
		}
		k273 = aptr;
		k607 += vl;
		aptr += vl;
		vl = 167;
	    }
	    else {
		/* dir$ ivdep */
		/* vdir nodep */
		/* VOCL LOOP, TEMP(t) */
		for (i = 1; i <= vl; ++i) {
		    t = a[k273 + i] + klotz0_1.buff[k607 + i - 1];
		    a[aptr + i] = t - (float)((int)t);
		}
		k607 += vl;
		k273 += vl;
		aptr += vl;
	    }
	}
	nn += -607;

	/* a(aptr-607) -> a(aptr) for last of the q-1 segments */

	aptr0 = aptr - 607;
	vl = 607;

	/* vdir novector */
	for (qq = 1; qq <= q - 2; ++qq) {
	    k273 = aptr0 + 334;
	    /* dir$ ivdep */
	    /* vdir nodep */
	    /* VOCL LOOP, TEMP(t), NOVREC(a) */
	    for (i = 1; i <= vl; ++i) {
		t = a[k273 + i] + a[aptr0 + i];
		a[aptr + i] = t - (float)((int)t);
	    }
	    nn += -607;
	    aptr += vl;
	    aptr0 += vl;
	}

	/* a(aptr0) -> buff, last segment before residual */

	vl = 273;
	k273 = aptr0 + 334;
	k607 = aptr0;
	bptr = 0;
	for (k = 1; k <= 3; ++k) {
	    if (k == 1) {
		/* VOCL LOOP, TEMP(t) */
		for (i = 1; i <= vl; ++i) {
		    t = a[k273 + i] + a[k607 + i];
		    klotz0_1.buff[bptr + i - 1] = t - (double)((int)t);
		}
		k273 = 0;
		k607 += vl;
		bptr += vl;
		vl = 167;
	    }
	    else {
		/* dir$ ivdep */
		/* vdir nodep */
		/* VOCL LOOP, TEMP(t), NOVREC(buff) */
		for (i = 1; i <= vl; ++i) {
		    t = klotz0_1.buff[k273 + i - 1] + a[k607 + i];
		    klotz0_1.buff[bptr + i - 1] = t - (float)((int)t);
		}
		k607 += vl;
		k273 += vl;
		bptr += vl;
	    }
	}
	goto L1;
    }
}				/* zufall */
