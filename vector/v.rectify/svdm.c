/*
   Copyright (c) 2003, Division of Imaging Science and Biomedical Engineering,
   University of Manchester, UK.  All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice, this list
   of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice, this
   list of conditions and the following disclaimer in the documentation and/or other
   materials provided with the distribution.

   Neither the name of the University of Manchester nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.


   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
   EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT
   SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>
#include <stdlib.h>

/* svd function */
#define SIGN(u, v)     ( (v)>=0.0 ? fabs(u) : -fabs(u) )
#define MAX(x, y)     ( (x) >= (y) ? (x) : (y) )

static double radius(double u, double v)
{
    double w;

    u = fabs(u);
    v = fabs(v);
    if (u > v) {
	w = v / u;
	return (u * sqrt(1. + w * w));
    }
    else {
	if (v) {
	    w = u / v;
	    return (v * sqrt(1. + w * w));
	}
	else
	    return 0.0;
    }
}

/*
   Given matrix a[m][n], m>=n, using svd decomposition a = p d q' to get
   p[m][n], diag d[n] and q[n][n].
 */
void svd(int m, int n, double **a, double **p, double *d, double **q)
{
    int flag, i, its, j, jj, k, l, nm, nm1 = n - 1, mm1 = m - 1;
    double c, f, h, s, x, y, z;
    double anorm = 0, g = 0, scale = 0;
    double *r = (double *)malloc(sizeof(double) * n);

    for (i = 0; i < m; i++)
	for (j = 0; j < n; j++)
	    p[i][j] = a[i][j];
    /*
       for (i = m; i < n; i++)
       p[i][j] = 0;
     */

    /* Householder reduction to bidigonal form */
    for (i = 0; i < n; i++) {
	l = i + 1;
	r[i] = scale * g;
	g = s = scale = 0.0;
	if (i < m) {
	    for (k = i; k < m; k++)
		scale += fabs(p[k][i]);
	    if (scale) {
		for (k = i; k < m; k++) {
		    p[k][i] /= scale;
		    s += p[k][i] * p[k][i];
		}
		f = p[i][i];
		g = -SIGN(sqrt(s), f);
		h = f * g - s;
		p[i][i] = f - g;
		if (i != nm1) {
		    for (j = l; j < n; j++) {
			for (s = 0.0, k = i; k < m; k++)
			    s += p[k][i] * p[k][j];
			f = s / h;
			for (k = i; k < m; k++)
			    p[k][j] += f * p[k][i];
		    }
		}
		for (k = i; k < m; k++)
		    p[k][i] *= scale;
	    }
	}
	d[i] = scale * g;
	g = s = scale = 0.0;
	if (i < m && i != nm1) {
	    for (k = l; k < n; k++)
		scale += fabs(p[i][k]);
	    if (scale) {
		for (k = l; k < n; k++) {
		    p[i][k] /= scale;
		    s += p[i][k] * p[i][k];
		}
		f = p[i][l];
		g = -SIGN(sqrt(s), f);
		h = f * g - s;
		p[i][l] = f - g;
		for (k = l; k < n; k++)
		    r[k] = p[i][k] / h;
		if (i != mm1) {
		    for (j = l; j < m; j++) {
			for (s = 0.0, k = l; k < n; k++)
			    s += p[j][k] * p[i][k];
			for (k = l; k < n; k++)
			    p[j][k] += s * r[k];
		    }
		}
		for (k = l; k < n; k++)
		    p[i][k] *= scale;
	    }
	}
	anorm = MAX(anorm, fabs(d[i]) + fabs(r[i]));
    }

    /* Accumulation of right-hand transformations */
    for (i = n - 1; i >= 0; i--) {
	if (i < nm1) {
	    if (g) {
		for (j = l; j < n; j++)
		    q[j][i] = (p[i][j] / p[i][l]) / g;
		for (j = l; j < n; j++) {
		    for (s = 0.0, k = l; k < n; k++)
			s += p[i][k] * q[k][j];
		    for (k = l; k < n; k++)
			q[k][j] += s * q[k][i];
		}
	    }
	    for (j = l; j < n; j++)
		q[i][j] = q[j][i] = 0.0;
	}
	q[i][i] = 1.0;
	g = r[i];
	l = i;
    }
    /* Accumulation of left-hand transformations */
    for (i = n - 1; i >= 0; i--) {
	l = i + 1;
	g = d[i];
	if (i < nm1)
	    for (j = l; j < n; j++)
		p[i][j] = 0.0;
	if (g) {
	    g = 1.0 / g;
	    if (i != nm1) {
		for (j = l; j < n; j++) {
		    for (s = 0.0, k = l; k < m; k++)
			s += p[k][i] * p[k][j];
		    f = (s / p[i][i]) * g;
		    for (k = i; k < m; k++)
			p[k][j] += f * p[k][i];
		}
	    }
	    for (j = i; j < m; j++)
		p[j][i] *= g;
	}
	else
	    for (j = i; j < m; j++)
		p[j][i] = 0.0;
	++p[i][i];
    }
    /* diagonalization of the bidigonal form */
    for (k = n - 1; k >= 0; k--) {	/* loop over singlar values */
	for (its = 0; its < 30; its++) {	/* loop over allowed iterations */
	    flag = 1;
	    for (l = k; l >= 0; l--) {	/* test for splitting */
		nm = l - 1;	/* note that r[l] is always
				 * zero */
		if (fabs(r[l]) + anorm == anorm) {
		    flag = 0;
		    break;
		}
		if (fabs(d[nm]) + anorm == anorm)
		    break;
	    }
	    if (flag) {
		c = 0.0;	/* cancellation of r[l], if
				 * l>1 */
		s = 1.0;
		for (i = l; i <= k; i++) {
		    f = s * r[i];
		    if (fabs(f) + anorm != anorm) {
			g = d[i];
			h = radius(f, g);
			d[i] = h;
			h = 1.0 / h;
			c = g * h;
			s = (-f * h);
			for (j = 0; j < m; j++) {
			    y = p[j][nm];
			    z = p[j][i];
			    p[j][nm] = y * c + z * s;
			    p[j][i] = z * c - y * s;
			}
		    }
		}
	    }
	    z = d[k];
	    if (l == k) {	/* convergence */
		if (z < 0.0) {
		    d[k] = -z;
		    for (j = 0; j < n; j++)
			q[j][k] = (-q[j][k]);
		}
		break;
	    }
	    if (its == 100) {
		/* svd: No convergence in 100 svd iterations, non_fatal error */
		return;
	    }
	    x = d[l];		/* shift from bottom 2-by-2 minor */
	    nm = k - 1;
	    y = d[nm];
	    g = r[nm];
	    h = r[k];
	    f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2.0 * h * y);
	    g = radius(f, 1.0);
	    /* next QR transformation */
	    f = ((x - z) * (x + z) + h * ((y / (f + SIGN(g, f))) - h)) / x;
	    c = s = 1.0;
	    for (j = l; j <= nm; j++) {
		i = j + 1;
		g = r[i];
		y = d[i];
		h = s * g;
		g = c * g;
		z = radius(f, h);
		r[j] = z;
		c = f / z;
		s = h / z;
		f = x * c + g * s;
		g = g * c - x * s;
		h = y * s;
		y = y * c;
		for (jj = 0; jj < n; jj++) {
		    x = q[jj][j];
		    z = q[jj][i];
		    q[jj][j] = x * c + z * s;
		    q[jj][i] = z * c - x * s;
		}
		z = radius(f, h);
		d[j] = z;	/* rotation can be arbitrary
				 * id z=0 */
		if (z) {
		    z = 1.0 / z;
		    c = f * z;
		    s = h * z;
		}
		f = (c * g) + (s * y);
		x = (c * y) - (s * g);
		for (jj = 0; jj < m; jj++) {
		    y = p[jj][j];
		    z = p[jj][i];
		    p[jj][j] = y * c + z * s;
		    p[jj][i] = z * c - y * s;
		}
	    }
	    r[l] = 0.0;
	    r[k] = f;
	    d[k] = x;
	}
    }
    free(r);

    /* dhli add: the original code does not sort the eigen value
       should do that and change the eigen vector accordingly
     */

}
