#include <grass/gis.h>
#include "local_proto.h"
#include <math.h>
double **matrix(), *vector();


/* Computes eigenvalues (and eigen vectors if desired) for      *
 *  symmetric matices.                                          */
int eigen(double **M,		/* Input matrix */
	  double *lambda,	/* Output eigenvalues */
	  int n			/* Input matrix dimension */
    )
{
    int i, j;
    double **a, *e;
    int stat;			/* 0 error, 1 ok */

    a = matrix(1, n, 1, n);
    lambda--;
    e = vector(1, n);

    for (i = 1; i <= n; i++)
	for (j = 1; j <= n; j++)
	    a[i][j] = M[i - 1][j - 1];

    tred2(a, n, lambda, e);
    stat = tqli(lambda, e, n, a);

    /* Returns eigenvectors */
    /*  for(i=1; i<=n; i++) 
       for(j=1; j<=n; j++) 
       M[i-1][j-1] = a[i][j]; */

    free_matrix(a, 1, n, 1, n);
    free_vector(e, 1, n);

    return stat;
}


#define SIGN(a,b) ((b)<0 ? -fabs(a) : fabs(a))

int tqli(double d[], double e[], int n, double **z)
{
    int m, l, iter, i, k;
    double s, r, p, g, f, dd, c, b;

    for (i = 2; i <= n; i++)
	e[i - 1] = e[i];
    e[n] = 0.0;
    for (l = 1; l <= n; l++) {
	iter = 0;
	do {
	    for (m = l; m <= n - 1; m++) {
		dd = fabs(d[m]) + fabs(d[m + 1]);
		if (fabs(e[m]) + dd == dd)
		    break;
	    }
	    if (m != l) {
		if (iter++ == 30)
		    return 0;	/*Too many iterations in TQLI */
		g = (d[l + 1] - d[l]) / (2.0 * e[l]);
		r = sqrt((g * g) + 1.0);
		g = d[m] - d[l] + e[l] / (g + SIGN(r, g));
		s = c = 1.0;
		p = 0.0;
		for (i = m - 1; i >= l; i--) {
		    f = s * e[i];
		    b = c * e[i];
		    if (fabs(f) >= fabs(g)) {
			c = g / f;
			r = sqrt((c * c) + 1.0);
			e[i + 1] = f * r;
			c *= (s = 1.0 / r);
		    }
		    else {
			s = f / g;
			r = sqrt((s * s) + 1.0);
			e[i + 1] = g * r;
			s *= (c = 1.0 / r);
		    }
		    g = d[i + 1] - p;
		    r = (d[i] - g) * s + 2.0 * c * b;
		    p = s * r;
		    d[i + 1] = g + p;
		    g = c * r - b;
		    /* Next loop can be omitted if eigenvectors not wanted */
		    for (k = 1; k <= n; k++) {
			f = z[k][i + 1];
			z[k][i + 1] = s * z[k][i] + c * f;
			z[k][i] = c * z[k][i] - s * f;
		    }
		}
		d[l] = d[l] - p;
		e[l] = g;
		e[m] = 0.0;
	    }
	} while (m != l);
    }
    return 1;
}



int tred2(double **a, int n, double d[], double e[])
{
    int l, k, j, i;
    double scale, hh, h, g, f;

    for (i = n; i >= 2; i--) {
	l = i - 1;
	h = scale = 0.0;
	if (l > 1) {
	    for (k = 1; k <= l; k++)
		scale += fabs(a[i][k]);
	    if (scale == 0.0)
		e[i] = a[i][l];
	    else {
		for (k = 1; k <= l; k++) {
		    a[i][k] /= scale;
		    h += a[i][k] * a[i][k];
		}
		f = a[i][l];
		g = f > 0 ? -sqrt(h) : sqrt(h);
		e[i] = scale * g;
		h -= f * g;
		a[i][l] = f - g;
		f = 0.0;
		for (j = 1; j <= l; j++) {
		    /* Next statement can be omitted if eigenvectors not wanted */
		    a[j][i] = a[i][j] / h;
		    g = 0.0;
		    for (k = 1; k <= j; k++)
			g += a[j][k] * a[i][k];
		    for (k = j + 1; k <= l; k++)
			g += a[k][j] * a[i][k];
		    e[j] = g / h;
		    f += e[j] * a[i][j];
		}
		hh = f / (h + h);
		for (j = 1; j <= l; j++) {
		    f = a[i][j];
		    e[j] = g = e[j] - hh * f;
		    for (k = 1; k <= j; k++)
			a[j][k] -= (f * e[k] + g * a[i][k]);
		}
	    }
	}
	else
	    e[i] = a[i][l];
	d[i] = h;
    }
    /* Next statement can be omitted if eigenvectors not wanted */
    d[1] = 0.0;
    e[1] = 0.0;
    /* Contents of this loop can be omitted if eigenvectors not
       wanted except for statement d[i]=a[i][i]; */
    for (i = 1; i <= n; i++) {
	l = i - 1;
	if (d[i]) {
	    for (j = 1; j <= l; j++) {
		g = 0.0;
		for (k = 1; k <= l; k++)
		    g += a[i][k] * a[k][j];
		for (k = 1; k <= l; k++)
		    a[k][j] -= g * a[k][i];
	    }
	}
	d[i] = a[i][i];
	a[i][i] = 1.0;
	for (j = 1; j <= l; j++)
	    a[j][i] = a[i][j] = 0.0;
    }

    return 0;
}
