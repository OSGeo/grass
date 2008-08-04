#include "zufall.h"
int zufalli(int *seed)
{
    static int kl = 9373;
    static int ij = 1802;

    static int i, j, k, l, m;
    static double s, t;
    static int ii, jj;
    extern struct klotz0 klotz0_1;

    /*  generates initial seed buffer by linear congruential */
    /*  method. Taken from Marsaglia, FSU report FSU-SCRI-87-50 */
    /*  variable seed should be 0 < seed <31328 */

    if (*seed != 0) {
	ij = *seed;
    }

    i = ij / 177 % 177 + 2;
    j = ij % 177 + 2;
    k = kl / 169 % 178 + 1;
    l = kl % 169;
    for (ii = 1; ii <= 607; ++ii) {
	s = (float)0.;
	t = (float).5;
	for (jj = 1; jj <= 24; ++jj) {
	    m = i * j % 179 * k % 179;
	    i = j;
	    j = k;
	    k = m;
	    l = (l * 53 + 1) % 169;
	    if (l * m % 64 >= 32) {
		s += t;
	    }
	    t *= (float).5;
	    /* L2: */
	}
	/*  fprintf (stdout,"DIAG: s %g\n",s); */
	klotz0_1.buff[ii - 1] = s;
	/* L1: */
    }
    return 0;
}				/* zufalli_ */
