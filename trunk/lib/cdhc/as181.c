#include <math.h>
#include <stdio.h>
#include <grass/cdhc.h>
#include "local_proto.h"


/* Local function prototypes */
static double poly(double c[], int nord, double x);


/*-Algorithm AS 181
 * by J.P. Royston, 1982.
 * Applied Statistics 31(2):176-180
 *
 * Translation to C by James Darrell McCauley, mccauley@ecn.purdue.edu.
 *
 * Calculates Shapiro and Wilk's W statistic and its sig. level
 *
 * Originally used:
 * Auxiliary routines required: Cdhc_alnorm = algorithm AS 66 and Cdhc_nscor2
 * from AS 177.

 * Note: ppnd() from as66 was replaced with ppnd16() from as241.
 */
void wext(double x[], int n, double ssq, double a[], int n2, double eps,
	  double *w, double *pw, int *ifault)
{
    double eu3, lamda, ybar, sdy, al, un, ww, y, z;
    int i, j, n3, nc;
    static double wa[3] = { 0.118898, 0.133414, 0.327907 };
    static double wb[4] = { -0.37542, -0.492145, -1.124332, -0.199422 };
    static double wc[4] = { -3.15805, 0.729399, 3.01855, 1.558776 };
    static double wd[6] = { 0.480385, 0.318828, 0.0, -0.0241665, 0.00879701,
	0.002989646
    };
    static double we[6] = { -1.91487, -1.37888, -0.04183209, 0.1066339,
	-0.03513666, -0.01504614
    };
    static double wf[7] = { -3.73538, -1.015807, -0.331885, 0.1773538,
	-0.01638782, -0.03215018, 0.003852646
    };
    static double unl[3] = { -3.8, -3.0, -1.0 };
    static double unh[3] = { 8.6, 5.8, 5.4 };
    static int nc1[3] = { 5, 5, 5 };
    static int nc2[3] = { 3, 4, 5 };
    double c[5];
    int upper = 1;
    static double pi6 = 1.90985932, stqr = 1.04719755;
    static double zero = 0.0, tqr = 0.75, one = 1.0;
    static double onept4 = 1.4, three = 3.0, five = 5.0;
    static double c1[5][3] = {
	{-1.26233, -2.28135, -3.30623},
	{1.87969, 2.26186, 2.76287},
	{0.0649583, 0.0, -0.83484},
	{-0.0475604, 0.0, 1.20857},
	{-0.0139682, -0.00865763, -0.507590}
    };
    static double c2[5][3] = {
	{-0.287696, -1.63638, -5.991908},
	{1.78953, 5.60924, 21.04575},
	{-0.180114, -3.63738, -24.58061},
	{0.0, 1.08439, 13.78661},
	{0.0, 0.0, -2.835295}
    };

    *ifault = 1;

    *pw = one;
    *w = one;

    if (n <= 2)
	return;

    *ifault = 3;
    if (n / 2 != n2)
	return;

    *ifault = 2;
    if (n > 2000)
	return;

    *ifault = 0;
    i = n - 1;

    for (*w = 0.0, j = 0; j < n2; ++j)
	*w += a[j] * (x[i--] - x[j]);

    *w *= *w / ssq;
    if (*w > one) {
	*w = one;

	return;
    }
    else if (n > 6) {		/* Get significance level of W */
	/*
	 * N between 7 and 2000 ... Transform W to Y, get mean and sd,
	 * standardize and get significance level
	 */

	if (n <= 20) {
	    al = log((double)n) - three;
	    lamda = poly(wa, 3, al);
	    ybar = exp(poly(wb, 4, al));
	    sdy = exp(poly(wc, 4, al));
	}
	else {
	    al = log((double)n) - five;
	    lamda = poly(wd, 6, al);
	    ybar = exp(poly(we, 6, al));
	    sdy = exp(poly(wf, 7, al));
	}

	y = pow(one - *w, lamda);
	z = (y - ybar) / sdy;
	*pw = Cdhc_alnorm(z, upper);

	return;
    }
    else {
	/* Deal with N less than 7 (Exact significance level for N = 3). */
	if (*w >= eps) {
	    ww = *w;
	    if (*w >= eps) {
		ww = *w;
		if (n == 3) {
		    *pw = pi6 * (atan(sqrt(ww / (one - ww))) - stqr);

		    return;
		}

		un = log((*w - eps) / (one - *w));
		n3 = n - 3;
		if (un >= unl[n3 - 1]) {
		    if (un <= onept4) {
			nc = nc1[n3 - 1];

			for (i = 0; i < nc; ++i)
			    c[i] = c1[i][n3 - 1];

			eu3 = exp(poly(c, nc, un));
		    }
		    else {
			if (un > unh[n3 - 1])
			    return;

			nc = nc2[n3 - 1];

			for (i = 0; i < nc; ++i)
			    c[i] = c2[i][n3 - 1];

			un = log(un);	/*alog */
			eu3 = exp(exp(poly(c, nc, un)));
		    }
		    ww = (eu3 + tqr) / (one + eu3);
		    *pw = pi6 * (atan(sqrt(ww / (one - ww))) - stqr);

		    return;
		}
	    }
	}
	*pw = zero;

	return;
    }

    return;
}


/*
 * Algorithm AS 181.1   Appl. Statist.  (1982) Vol. 31, No. 2
 * 
 * Obtain array A of weights for calculating W
 */
void wcoef(double a[], int n, int n2, double *eps, int *ifault)
{
    static double c4[2] = { 0.6869, 0.1678 };
    static double c5[2] = { 0.6647, 0.2412 };
    static double c6[3] = { 0.6431, 0.2806, 0.0875 };
    static double rsqrt2 = 0.70710678;
    double a1star, a1sq, sastar, an;
    int j;

    *ifault = 1;
    if (n <= 2)
	return;

    *ifault = 3;
    if (n / 2 != n2)
	return;

    *ifault = 2;
    if (n > 2000)
	return;

    *ifault = 0;
    if (n > 6) {
	/* Calculate rankits using approximate function Cdhc_nscor2().  (AS177) */
	Cdhc_nscor2(a, n, n2, ifault);

	for (sastar = 0.0, j = 1; j < n2; ++j)
	    sastar += a[j] * a[j];

	sastar *= 8.0;

	an = n;
	if (n <= 20)
	    an--;
	a1sq = exp(log(6.0 * an + 7.0) - log(6.0 * an + 13.0)
		   + 0.5 * (1.0 + (an - 2.0) * log(an + 1.0) - (an - 1.0)
			    * log(an + 2.0)));
	a1star = sastar / (1.0 / a1sq - 2.0);
	sastar = sqrt(sastar + 2.0 * a1star);
	a[0] = sqrt(a1star) / sastar;

	for (j = 1; j < n2; ++j)
	    a[j] = 2.0 * a[j] / sastar;
    }
    else {
	/* Use exact values for weights */

	a[0] = rsqrt2;
	if (n != 3) {
	    if (n - 3 == 3)
		for (j = 0; j < 3; ++j)
		    a[j] = c6[j];
	    else if (n - 3 == 2)
		for (j = 0; j < 2; ++j)
		    a[j] = c5[j];
	    else
		for (j = 0; j < 2; ++j)
		    a[j] = c4[j];

      /*-
            goto (40,50,60), n3
         40 do 45 j = 1,2
         45 a(j) = c4(j)
            goto 70
         50 do 55 j = 1,2
         55 a(j) = c5(j)
            goto 70
         60 do 65 j = 1,3
         65 a(j) = c6(j)
      */
	}
    }

    /* Calculate the minimum possible value of W */
    *eps = a[0] * a[0] / (1.0 - 1.0 / (double)n);

    return;
}


/*
 * Algorithm AS 181.2   Appl. Statist.  (1982) Vol. 31, No. 2
 * 
 * Calculates the algebraic polynomial of order nored-1 with array of
 * coefficients c.  Zero order coefficient is c(1)
 */
static double poly(double c[], int nord, double x)
{
    double p;
    int n2, i, j;

    if (nord == 1)
	return c[0];

    p = x * c[nord - 1];

    if (nord != 2) {
	n2 = nord - 2;
	j = n2;

	for (i = 0; i < n2; ++i)
	    p = (p + c[j--]) * x;
    }

    return c[0] + p;
}


/*
 * AS R63 Appl. Statist. (1986) Vol. 35, No.2
 * 
 * A remark on AS 181
 * 
 * Calculates Sheppard Cdhc_corrected version of W test.
 * 
 * Auxiliary functions required: Cdhc_alnorm = algorithm AS 66, and PPND =
 * algorithm AS 111 (or Cdhc_ppnd7 from AS 241).
 */
void Cdhc_wgp(double x[], int n, double ssq, double gp, double h, double a[],
	 int n2, double eps, double w, double u, double p, int *ifault)
{
    double zbar, zsd, an1, hh;

    zbar = 0.0;
    zsd = 1.0;
    *ifault = 1;

    if (n < 7)
	return;

    if (gp > 0.0) {		/* No Cdhc_correction applied if gp=0. */
	an1 = (double)(n - 1);
	/* Cdhc_correct ssq and find standardized grouping interval (h) */
	ssq = ssq - an1 * gp * gp / 12.0;
	h = gp / sqrt(ssq / an1);
	*ifault = 4;

	if (h > 1.5)
	    return;
    }
    wext(x, n, ssq, a, n2, eps, &w, &p, ifault);

    if (*ifault != 0)
	return;

    if (!(p > 0.0 && p < 1.0)) {
	u = 5.0 - 10.0 * p;

	return;
    }

    if (gp > 0.0) {
	/* Cdhc_correct u for grouping interval (n<=100 and n>100 separately) */
	hh = sqrt(h);
	if (n <= 100) {
	    zbar = -h * (1.07457 + hh * (-2.8185 + hh * 1.8898));
	    zsd = 1.0 + h * (0.50933 + hh * (-0.98305 + hh * 0.7408));
	}
	else {
	    zbar = -h * (0.96436 + hh * (-2.1300 + hh * 1.3196));
	    zsd = 1.0 + h * (0.2579 + h * 0.15225);
	}
    }

    /* ppnd is AS 111 (Beasley and Springer, 1977) */
    u = (-ppnd16(p) - zbar) / zsd;

    /* Cdhc_alnorm is AS 66 (Hill, 1973) */
    p = Cdhc_alnorm(u, 1);

    return;
}
