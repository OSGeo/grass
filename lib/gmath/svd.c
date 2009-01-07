#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>

static double at, bt, ct;

#define PYTHAG(a,b) ((at=fabs(a)) > (bt=fabs(b)) ? \
    (ct=bt/at,at*sqrt(1.0+ct*ct)) : (bt ? (ct=at/bt,bt*sqrt(1.0+ct*ct)): 0.0))

static double maxarg1, maxarg2;

#define MAX(a,b) (maxarg1=(a),maxarg2=(b),(maxarg1) > (maxarg2) ?\
	(maxarg1) : (maxarg2))
#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

int G_svdcmp(double **a, int m, int n, double *w, double **v)
{
    int flag, i, its, j, jj, k, ii = 0, nm = 0;
    double c, f, h, s, x, y, z;
    double anorm = 0.0, g = 0.0, scale = 0.0;
    double *rv1, *G_alloc_vector();


    if (m < n)
	return -1;		/* must augment A with extra zero rows */
    rv1 = G_alloc_vector(n);

    n--;
    m--;

    for (i = 0; i <= n; i++) {
	ii = i + 1;
	rv1[i] = scale * g;
	g = s = scale = 0.0;
	if (i <= m) {
	    for (k = i; k <= m; k++)
		scale += fabs(a[k][i]);
	    if (scale) {
		for (k = i; k <= m; k++) {
		    a[k][i] /= scale;
		    s += a[k][i] * a[k][i];
		}
		f = a[i][i];
		g = -SIGN(sqrt(s), f);
		h = f * g - s;
		a[i][i] = f - g;
		if (i != n) {
		    for (j = ii; j <= n; j++) {
			for (s = 0.0, k = i; k <= m; k++)
			    s += a[k][i] * a[k][j];
			f = s / h;
			for (k = i; k <= m; k++)
			    a[k][j] += f * a[k][i];
		    }
		}
		for (k = i; k <= m; k++)
		    a[k][i] *= scale;
	    }
	}
	w[i] = scale * g;
	g = s = scale = 0.0;
	if (i <= m && i != n) {
	    for (k = ii; k <= n; k++)
		scale += fabs(a[i][k]);
	    if (scale) {
		for (k = ii; k <= n; k++) {
		    a[i][k] /= scale;
		    s += a[i][k] * a[i][k];
		}
		f = a[i][ii];
		g = -SIGN(sqrt(s), f);
		h = f * g - s;
		a[i][ii] = f - g;
		for (k = ii; k <= n; k++)
		    rv1[k] = a[i][k] / h;
		if (i != m) {
		    for (j = ii; j <= m; j++) {
			for (s = 0.0, k = ii; k <= n; k++)
			    s += a[j][k] * a[i][k];
			for (k = ii; k <= n; k++)
			    a[j][k] += s * rv1[k];
		    }
		}
		for (k = ii; k <= n; k++)
		    a[i][k] *= scale;
	    }
	}
	anorm = MAX(anorm, (fabs(w[i]) + fabs(rv1[i])));
    }
    for (i = n; i >= 0; i--) {
	if (i < n) {
	    if (g) {
		for (j = ii; j <= n; j++)
		    v[j][i] = (a[i][j] / a[i][ii]) / g;
		for (j = ii; j <= n; j++) {
		    for (s = 0.0, k = ii; k <= n; k++)
			s += a[i][k] * v[k][j];
		    for (k = ii; k <= n; k++)
			v[k][j] += s * v[k][i];
		}
	    }
	    for (j = ii; j <= n; j++)
		v[i][j] = v[j][i] = 0.0;
	}
	v[i][i] = 1.0;
	g = rv1[i];
	ii = i;
    }
    for (i = n; i >= 0; i--) {
	ii = i + 1;
	g = w[i];
	if (i < n)
	    for (j = ii; j <= n; j++)
		a[i][j] = 0.0;
	if (g) {
	    g = 1.0 / g;
	    if (i != n) {
		for (j = ii; j <= n; j++) {
		    for (s = 0.0, k = ii; k <= m; k++)
			s += a[k][i] * a[k][j];
		    f = (s / a[i][i]) * g;
		    for (k = i; k <= m; k++)
			a[k][j] += f * a[k][i];
		}
	    }
	    for (j = i; j <= m; j++)
		a[j][i] *= g;
	}
	else {
	    for (j = i; j <= m; j++)
		a[j][i] = 0.0;
	}
	++a[i][i];
    }
    for (k = n; k >= 0; k--) {
	for (its = 1; its <= 30; its++) {
	    flag = 1;
	    for (ii = k; ii >= 0; ii--) {
		nm = ii - 1;
		if (fabs(rv1[ii]) + anorm == anorm) {
		    flag = 0;
		    break;
		}
		if (fabs(w[nm]) + anorm == anorm)
		    break;
	    }
	    if (flag) {
		c = 0.0;
		s = 1.0;
		for (i = ii; i <= k; i++) {
		    f = s * rv1[i];
		    if (fabs(f) + anorm != anorm) {
			g = w[i];
			h = PYTHAG(f, g);
			w[i] = h;
			h = 1.0 / h;
			c = g * h;
			s = (-f * h);
			for (j = 0; j <= m; j++) {
			    y = a[j][nm];
			    z = a[j][i];
			    a[j][nm] = y * c + z * s;
			    a[j][i] = z * c - y * s;
			}
		    }
		}
	    }
	    z = w[k];
	    if (ii == k) {
		if (z < 0.0) {
		    w[k] = -z;
		    for (j = 0; j <= n; j++)
			v[j][k] = (-v[j][k]);
		}
		break;
	    }
	    if (its == 30)
		return -2;	/*No convergence in 30 SVDCMP iterations */
	    x = w[ii];
	    nm = k - 1;
	    y = w[nm];
	    g = rv1[nm];
	    h = rv1[k];
	    f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2.0 * h * y);
	    g = PYTHAG(f, 1.0);
	    f = ((x - z) * (x + z) + h * ((y / (f + SIGN(g, f))) - h)) / x;
	    c = s = 1.0;
	    for (j = ii; j <= nm; j++) {
		i = j + 1;
		g = rv1[i];
		y = w[i];
		h = s * g;
		g = c * g;
		z = PYTHAG(f, h);
		rv1[j] = z;
		c = f / z;
		s = h / z;
		f = x * c + g * s;
		g = g * c - x * s;
		h = y * s;
		y = y * c;
		for (jj = 0; jj <= n; jj++) {
		    x = v[jj][j];
		    z = v[jj][i];
		    v[jj][j] = x * c + z * s;
		    v[jj][i] = z * c - x * s;
		}
		z = PYTHAG(f, h);
		w[j] = z;
		if (z) {
		    z = 1.0 / z;
		    c = f * z;
		    s = h * z;
		}
		f = (c * g) + (s * y);
		x = (c * y) - (s * g);
		for (jj = 0; jj <= m; jj++) {
		    y = a[jj][j];
		    z = a[jj][i];
		    a[jj][j] = y * c + z * s;
		    a[jj][i] = z * c - y * s;
		}
	    }
	    rv1[ii] = 0.0;
	    rv1[k] = f;
	    w[k] = x;
	}
    }
    G_free_vector(rv1);
    return 0;
}

#undef SIGN
#undef MAX
#undef PYTHAG

int G_svbksb(double **u, double w[], double **v,
	     int m, int n, double b[], double x[])
{
    int j, i;
    double s, *tmp, *G_alloc_vector();

    tmp = G_alloc_vector(n);
    for (j = 0; j < n; j++) {
	s = 0.0;
	if (w[j]) {
	    for (i = 0; i < m; i++)
		s += u[i][j] * b[i];
	    s /= w[j];
	}
	tmp[j] = s;
    }
    for (j = 0; j < n; j++) {
	s = 0.0;
	for (i = 0; i < n; i++)
	    s += v[j][i] * tmp[i];
	x[j] = s;
    }
    G_free_vector(tmp);

    return 0;
}

#define TOL 1e-8

int G_svelim(double *w, int n)
{
    int i;
    double thresh;

    thresh = 0.0;		/* remove singularity */
    for (i = 0; i < n; i++)
	if (w[i] > thresh)
	    thresh = w[i];
    thresh *= TOL;
    for (i = 0; i < n; i++)
	if (w[i] < thresh)
	    w[i] = 0.0;

    return 0;
}

#undef TOL
