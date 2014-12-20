/*!
 * \file lib/gis/geodist.c
 *
 * \brief GIS Library - Geodesic distance routines.
 *
 * Distance from point to point along a geodesic code from Paul
 * D. Thomas, 1970 "Spheroidal Geodesics, Reference Systems, and Local
 * Geometry" U.S. Naval Oceanographic Office, p. 162 Engineering
 * Library 526.3 T36s
 * http://stinet.dtic.mil/oai/oai?&verb=getRecord&metadataPrefix=html&identifier=AD0703541
 *
 * <b>WARNING:</b> this code is preliminary and may be changed,
 * including calling sequences to any of the functions defined here.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <math.h>
#include <grass/gis.h>
#include "pi.h"

static struct state {
    double boa;
    double f;
    double ff64;
    double al;
    double t1, t2, t3, t4, t1r, t2r;
} state;

static struct state *st = &state;

/*!
 * \brief Begin geodesic distance.
 *
 * Initializes the distance calculations for the ellipsoid with
 * semi-major axis <i>a</i> (in meters) and ellipsoid eccentricity squared
 * <i>e2</i>. It is used only for the latitude-longitude projection.
 *
 * <b>Note:</b> Must be called once to establish the ellipsoid.
 *
 * \param a semi-major axis in meters
 * \param e2 ellipsoid eccentricity
 */

void G_begin_geodesic_distance(double a, double e2)
{
    st->al = a;
    st->boa = sqrt(1 - e2);
    st->f = 1 - st->boa;
    st->ff64 = st->f * st->f / 64;
}

/*!
 * \brief Sets geodesic distance lat1.
 *
 * Set the first latitude.
 *
 * <b>Note:</b> Must be called first.
 *
 * \param lat1 first latitude
 * \return
 */

void G_set_geodesic_distance_lat1(double lat1)
{
    st->t1r = atan(st->boa * tan(Radians(lat1)));
}

/*!
 * \brief Sets geodesic distance lat2.
 *
 * Set the second latitude.
 *
 * <b>Note:</b> Must be called second.
 *
 * \param lat2 second latitidue
 */
void G_set_geodesic_distance_lat2(double lat2)
{
    double stm, ctm, sdtm, cdtm;
    double tm, dtm;

    st->t2r = atan(st->boa * tan(Radians(lat2)));

    tm = (st->t1r + st->t2r) / 2;
    dtm = (st->t2r - st->t1r) / 2;

    stm = sin(tm);
    ctm = cos(tm);
    sdtm = sin(dtm);
    cdtm = cos(dtm);

    st->t1 = stm * cdtm;
    st->t1 = st->t1 * st->t1 * 2;

    st->t2 = sdtm * ctm;
    st->t2 = st->t2 * st->t2 * 2;

    st->t3 = sdtm * sdtm;
    st->t4 = cdtm * cdtm - stm * stm;
}

/*!
 * \brief Calculates geodesic distance.
 *
 * Calculates the geodesic distance from <i>lon1,lat1</i> to
 * <i>lon2,lat2</i> in meters where <i>lat1</i> was the latitude
 * passed to G_set_geodesic_distance_latl() and <i>lat2</i> was the
 * latitude passed to G_set_geodesic_distance_lat2().
 *
 * \param lon1 first longitude
 * \param lon2 second longitude
 *
 * \return double distance in meters
 */
double G_geodesic_distance_lon_to_lon(double lon1, double lon2)
{
    double a, cd, d, e,		/*dl, */
      q, sd, sdlmr, t, u, v, x, y;


    sdlmr = sin(Radians(lon2 - lon1) / 2);

    /* special case - shapiro */
    if (sdlmr == 0.0 && st->t1r == st->t2r)
	return 0.0;

    q = st->t3 + sdlmr * sdlmr * st->t4;

    /* special case - shapiro */
    if (q == 1.0)
	return M_PI * st->al;

    /* Mod: shapiro
     * cd=1-2q is ill-conditioned if q is small O(10**-23)
     *   (for high lats? with lon1-lon2 < .25 degrees?)
     *   the computation of cd = 1-2*q will give cd==1.0.
     * However, note that t=dl/sd is dl/sin(dl) which approaches 1 as dl->0.
     * So the first step is to compute a good value for sd without using sin()
     *   and then check cd && q to see if we got cd==1.0 when we shouldn't.
     * Note that dl isn't used except to get t,
     *   but both cd and sd are used later
     */

    /* original code
       cd=1-2*q;
       dl=acos(cd);
       sd=sin(dl);
       t=dl/sd;
     */

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

    u = st->t1 / (1 - q);
    v = st->t2 / q;
    d = 4 * t * t;
    x = u + v;
    e = -2 * cd;
    y = u - v;
    a = -d * e;

    return st->al * sd * (t
			  - st->f / 4 * (t * x - y)
			  + st->ff64 * (x * (a + (t - (a + e) / 2) * x)
					+ y * (-2 * d + e * y) + d * x * y));
}

/*!
 * \brief Calculates geodesic distance.
 *
 * Calculates the geodesic distance from <i>lon1,lat1</i> to 
 * <i>lon2,lat2</i> in meters.
 * 
 * <b>Note:</b> The calculation of the geodesic distance is fairly
 * costly.
 *
 * \param lon1,lat1 longitude,latitude of first point
 * \param lon2,lat2 longitude,latitude of second point
 *
 * \return distance in meters
 */
double G_geodesic_distance(double lon1, double lat1, double lon2, double lat2)
{
    G_set_geodesic_distance_lat1(lat1);
    G_set_geodesic_distance_lat2(lat2);
    return G_geodesic_distance_lon_to_lon(lon1, lon2);
}
