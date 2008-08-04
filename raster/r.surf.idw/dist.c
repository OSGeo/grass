#include <math.h>
#include <grass/gis.h>
#include "pi.h"

/* distance from point to point along a geodesic 
 * code from
 *   Paul D. Thomas
 *   "Spheroidal Geodesics, Reference Systems, and Local Geometry"
 *   U.S. Naval Oceanographic Office, p. 162
 *   Engineering Library 526.3 T36s
 */

/* a corruption of geodist.c library module of GRASS, used to enhance
   execution efficiency of interpolation routine */

static double boa;
static double f;
static double ff64;
static double al;

static double t1r, t2r;

#define DIST_PARAMS	struct dist_params
DIST_PARAMS {
    short targetrow;		/* interpolation row for which params apply */
    double t1, t2, t3, t4;
};

static DIST_PARAMS *lat_params, *nextcalc;


/* must be called once to establish the ellipsoid */
int G_begin_geodesic_distance_l(short nrows, double a, double e2)
{
    int i;

    al = a;
    boa = sqrt(1 - e2);
    f = 1 - boa;
    ff64 = f * f / 64;

    /* initialize lat_params array and indicate no prior data storage */
    lat_params = (DIST_PARAMS *) G_calloc(nrows, sizeof(DIST_PARAMS));
    for (i = 0, nextcalc = lat_params; i < nrows; i++, nextcalc++)
	nextcalc->targetrow = -1;

    return 0;
}


double LL_set_geodesic_distance_lat(double lat)
{
    return (atan(boa * tan(Radians(lat))));
}


double set_sdlmr(double lon_diff)
{
    return (sin(Radians(lon_diff) / 2));
}


/* must be called first */
int LL_set_geodesic_distance(double *rowlook,	/* preprocessed latitude data by row */
			     int unk, int data	/* row (y) of interpolation target, data value */
    )
{
    double stm, ctm, sdtm, cdtm;
    double tm, dtm;
    double temp;

    t1r = *(rowlook + unk);
    t2r = *(rowlook + data);

    tm = (t1r + t2r) / 2;
    dtm = (t2r - t1r) / 2;

    stm = sin(tm);
    ctm = cos(tm);
    sdtm = sin(dtm);
    cdtm = cos(dtm);

    nextcalc = lat_params + data;
    if (nextcalc->targetrow != unk) {	/* reset latitude offset parameters */
	temp = stm * cdtm;
	nextcalc->t1 = temp * temp * 2;

	temp = sdtm * ctm;
	nextcalc->t2 = temp * temp * 2;

	nextcalc->t3 = sdtm * sdtm;
	nextcalc->t4 = cdtm * cdtm - stm * stm;

	nextcalc->targetrow = unk;	/* parameterization tagged to row */
    }

    return 0;
}


double LL_geodesic_distance(double sdlmr)
{
    double a, cd, d, e, q, sd, t, u, v, x, y;

    /* special case - shapiro */
    if (sdlmr == 0.0 && t1r == t2r)
	return 0.0;

    q = nextcalc->t3 + sdlmr * sdlmr * nextcalc->t4;
    /* special case - shapiro */
    if (q == 1.0)
	return PI * al;

    cd = 1 - 2 * q;		/* ill-conditioned subtraction for small q */
    /* mod starts here */
    sd = 2 * sqrt(q - q * q);	/* sd^2 = 1 - cd^2 */
    if (q != 0.0 && cd == 1.0)	/* test for small q */
	t = 1.0;
    else if (sd == 0.0)
	t = 1.0;
    else
	t = acos(cd) / sd;	/* don't know how to fix acos(1-2*q) yet */
    /* mod ends here */

    u = nextcalc->t1 / (1 - q);
    v = nextcalc->t2 / q;
    d = 4 * t * t;
    x = u + v;
    e = -2 * cd;
    y = u - v;
    a = -d * e;

    return (al * sd *
	    (t - f / 4 * (t * x - y) +
	     ff64 * (x * (a + (t - (a + e) / 2) * x) + y * (-2 * d + e * y)
		     + d * x * y)
	    )
	);
}


int free_dist_params(void)
{
    G_free(lat_params);

    return 0;
}
