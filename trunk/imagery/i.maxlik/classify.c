#include <grass/gis.h>
#include <grass/raster.h>
#include "global.h"


static double chisq[] =
    { 18.465, 14.860, 13.277, 11.668, 9.488, 7.779, 5.989, 4.878,
    3.357, 2.195, 1.649, 1.064, 0.711, 0.429, 0.297, 0.0
};


int classify(CELL * class, CELL * reject, int ncols)
{
    int i, j;
    int nfiles = Ref.nfiles;
    int c;
    int cc = 0;
    int band;
    int col;
    int valid_data;
    double max, tot, sum;
    double rej;
    struct One_Sig *s;

    for (col = 0; col < ncols; col++) {
	valid_data = 0;
	for (band = 0; band < nfiles; band++)
	    if ((valid_data = !Rast_is_d_null_value(&cell[band][col])))
		break;

	if (!valid_data) {	/* all nulls are classified as nulls */
	    Rast_set_c_null_value(class++, 1);
	    if (reject)
		Rast_set_c_null_value(reject++, 1);
	    continue;
	}

	max = -1.0e38;
	for (c = 0; c < S.nsigs; c++) {
	    /* The following test is designed to speed up the search for the
	     * most probable class.  In fact if the B[] array is sorted, the
	     * search  could  be terminated sooner.
	     */
	    if (B[c] <= max)
		continue;

	    /*
	       The test only works if  the  covariance  matrix  is  non-negative
	       definite (sometimes  called positve semi-definite), and this is a
	       requirement of the maximum-likelihood estimator.  This assumption
	       is  theorically  true  for random samples of normally distributed
	       data, but for imagery data this is not generally the  case.   The
	       matrix  inversion/determinanat  routine  should  enforce positive
	       semi-definiteness. I could not tell if  it  did  this.   I  don't
	       think  it does.  A necessary condition is that the determinant be
	       positive but this is not sufficient. All  principal  minors  must
	       also have non-negative determinants.
	     */


	    s = &S.sig[c];

	    tot = 0.0;
	    for (band = 0; band < nfiles; band++) {
		double p;

		p = P[band] = cell[band][col] - s->mean[band];
		tot += p * p * s->var[band][band];
	    }
	    tot = B[c] - 0.5 * tot;

	    sum = 0.0;
	    for (i = 0; i < nfiles - 1; i++)
		for (j = i + 1; j < nfiles; j++)
		    sum += P[j] * P[i] * s->var[j][i];
	    tot -= sum;
	    if (tot > max) {
		cc = c;
		max = tot;
	    }
	}
	*class++ = cc + 1;

	if (reject) {
	    rej = 2 * (B[cc] - max);
	    for (i = 0; i < 16; i++)
		if (rej >= chisq[i])
		    break;
	    *reject++ = i + 1;
	}
    }

    return 0;
}
