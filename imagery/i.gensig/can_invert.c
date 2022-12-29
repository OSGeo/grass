#include <math.h>
#include <grass/gis.h>
#include "local_proto.h"

int can_invert(double **a, int n)
{
    int i, imax = 0, j, k;
    double big, dum, sum, temp;
    double *vv;

    vv = (double *)G_calloc(n, sizeof(double));
    for (i = 0; i < n; i++) {
	big = 0.0;
	for (j = 0; j < n; j++)
	    if ((temp = fabs(a[i][j])) > big)
		big = temp;
	if (big == 0.0)
	    goto singular;
	vv[i] = 1.0 / big;
    }
    for (j = 0; j < n; j++) {
	for (i = 0; i < j; i++) {
	    sum = a[i][j];
	    for (k = 0; k < i; k++)
		sum -= a[i][k] * a[k][j];
	    a[i][j] = sum;
	}
	big = 0.0;
	for (i = j; i < n; i++) {
	    sum = a[i][j];
	    for (k = 0; k < j; k++)
		sum -= a[i][k] * a[k][j];
	    a[i][j] = sum;
	    if ((dum = vv[i] * fabs(sum)) >= big) {
		big = dum;
		imax = i;
	    }
	}
	if (j != imax) {
	    for (k = 0; k < n; k++) {
		dum = a[imax][k];
		a[imax][k] = a[j][k];
		a[j][k] = dum;
	    }
	    vv[imax] = vv[j];
	}
	if (a[j][j] == 0.0)
	    goto singular;
	if (j != n - 1) {
	    dum = 1.0 / (a[j][j]);
	    for (i = j; i < n; i++)
		a[i][j] *= dum;
	}
    }
    G_free(vv);
    return 1;
  singular:
    G_free(vv);
    return 0;
}

#undef TINY
