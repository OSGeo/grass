
/*-Algorithm AS 177
 * Expected Normal Order Statistics (Exact and Approximate),
 * by J.P. Royston, 1982.
 * Applied Statistics, 31(2):161-165.
 *
 * Translation to C by James Darrell McCauley, mccauley@ecn.purdue.edu.
 *
 * The functions Cdhc_nscor1() and Cdhc_nscor2() calculate the expected values of
 * normal order statistics in exact or approximate form, respectively.
 *
 */

#define NSTEP 721
#define H 0.025

#include <math.h>
#include <stdio.h>
#include "local_proto.h"


/* Local function prototypes */
static double Cdhc_alnfac(int j);
static double Cdhc_correc(int i, int n);


/* exact calculation of normal scores */
void Cdhc_nscor1(double s[], int n, int n2, double work[], int *ifault)
{
    double ani, c, c1, d, scor;
    int i, j;

    *ifault = 3;
    if (n2 != n / 2)
	return;

    *ifault = 1;
    if (n <= 1)
	return;

    *ifault = 0;
    if (n > 2000)
	*ifault = 2;

    /* calculate the natural log of factorial(n) */
    c1 = Cdhc_alnfac(n);
    d = c1 - log((double)n);

    /* accumulate ordinates for calculation of integral for rankits */
    for (i = 0; i < n2; ++i) {
	ani = (double)n - i - 1;
	c = c1 - d;
	for (scor = 0.0, j = 0; j < NSTEP; ++j)
	    scor += work[0 * NSTEP + j] *
		exp(work[1 * NSTEP + j] + work[2 * NSTEP + j] * i
		    + work[3 * NSTEP + j] * ani + c);
	s[i] = scor * H;
	d += log((double)(i + 1.0) / ani);
    }

    return;
}


void init(double work[])
{
    double xstart = -9.0, pi2 = -0.918938533, xx;
    int i;

    xx = xstart;

    /* set up arrays for calculation of integral */
    for (i = 0; i < NSTEP; ++i) {
	work[0 * NSTEP + i] = xx;
	work[1 * NSTEP + i] = pi2 - xx * xx * 0.5;
	work[2 * NSTEP + i] = log(Cdhc_alnorm(xx, 1));
	work[3 * NSTEP + i] = log(Cdhc_alnorm(xx, 0));
	xx = xstart + H * (i + 1.0);
    }

    return;
}


/*-Algorithm AS 177.2 Appl. Statist. (1982) Vol.31, No.2
 * Natural logarithm of factorial for non-negative argument
 */
static double Cdhc_alnfac(int j)
{
    static double r[7] = { 0.0, 0.0, 0.69314718056, 1.79175946923,
	3.17805383035, 4.78749174278, 6.57925121101
    };
    double w, z;

    if (j == 1)
	return (double)1.0;
    else if (j <= 7)
	return r[j];

    w = (double)j + 1;
    z = 1.0 / (w * w);

    return (w - 0.5) * log(w) - w + 0.918938522305 +
	(((4.0 - 3.0 * z) * z - 14.0) * z + 420.0) / (5040.0 * w);
}


/*-Algorithm AS 177.3 Appl. Statist. (1982) Vol.31, No.2
 * Approximation for Rankits
 */
void Cdhc_nscor2(double s[], int n, int n2, int *ifault)
{
    static double eps[4] = { 0.419885, 0.450536, 0.456936, 0.468488 };
    static double dl1[4] = { 0.112063, 0.121770, 0.239299, 0.215159 };
    static double dl2[4] = { 0.080122, 0.111348, -0.211867, -0.115049 };
    static double gam[4] = { 0.474798, 0.469051, 0.208597, 0.259784 };
    static double lam[4] = { 0.282765, 0.304856, 0.407708, 0.414093 };
    static double bb = -0.283833, d = -0.106136, b1 = 0.5641896;
    double e1, e2, l1;
    int i, k;

    *ifault = 3;
    if (n2 != n / 2)
	return;

    *ifault = 1;
    if (n <= 1)
	return;

    *ifault = 0;
    if (n > 2000)
	*ifault = 2;

    s[0] = b1;
    if (n == 2)
	return;

    /* calculate normal areas for 3 largest rankits */
    k = (n2 < 3) ? n2 : 3;
    for (i = 0; i < k; ++i) {
	e1 = (1.0 + i - eps[i]) / (n + gam[i]);
	e2 = pow(e1, lam[i]);
	s[i] = e1 + e2 * (dl1[i] + e2 * dl2[i]) / n - Cdhc_correc(1 + i, n);
    }

    if (n2 != k) {
	/* calculate normal areas for remaining rankits */
	for (i = 3; i < n2; ++i) {
	    l1 = lam[3] + bb / (1.0 + i + d);
	    e1 = (1.0 + i - eps[3]) / (n + gam[3]);
	    e2 = pow(e1, l1);
	    s[i] = e1 + e2 * (dl1[3] + e2 * dl2[3]) / n - Cdhc_correc(1 + i, n);
	}
    }

    /* convert normal tail areas to normal deviates */
    for (i = 0; i < n2; ++i)
	s[i] = -ppnd16(s[i]);

    return;
}


/*-Algorithm AS 177.4 Appl. Statist. (1982) Vol.31, No.2
 * Calculates Cdhc_correction for tail area of noraml distribution
 * corresponding to ith largest rankit in sample size n.
 */
static double Cdhc_correc(int i, int n)
{
    static double c1[7] = { 9.5, 28.7, 1.9, 0.0, -7.0, -6.2, -1.6 };
    static double c2[7] = { -6.195e3, -9.569e3, -6.728e3, -17.614e3,
	-8.278e3, -3.570e3, 1.075e3
    };
    static double c3[7] = { 9.338e4, 1.7516e5, 4.1040e5, 2.157e6,
	2.376e6, 2.065e6, 2.065e6
    };
    static double mic = 1.0e-6, c14 = 1.9e-5;
    double an;

    if (i * n == 4)
	return c14;

    if (i < 1 || i > 7)
	return 0.0;
    else if (i != 4 && n > 20)
	return 0.0;
    else if (i == 4 && n > 40)
	return 0.0;

    /* else */
    an = 1.0 / (double)(n * n);
    return (c1[i - 1] + an * (c2[i - 1] + an * c3[i - 1])) * mic;
}
