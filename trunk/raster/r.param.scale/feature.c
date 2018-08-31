/* changes line  37 for Linux - Markus Neteler (Jan. 1998) */

/*****************************************************************************/

/***                                                                       ***/

/***                              feature()                                ***/

/***     Returns a terrain feature based on the 6 quadratic coefficients    ***/

/***	 that define a local trend surface. 			    	   ***/

/***     Jo Wood, Department of Geography, V2.1 30th March, 1995           ***/

/***                                                                       ***/

/*****************************************************************************/

#include "param.h"
#include <math.h>


DCELL feature(double *coeff)
{				/* Set of six quadratic coefficients.      */

    /* Quadratic function in the form of

       z = ax^2 + by^2 + cxy + dx + ey +f                       */

    double a = C_A * zscale,	/* Scale parameters if necessary.       */
	b = C_B * zscale,
	c = C_C * zscale, d = C_D * zscale, e = C_E * zscale;

    double maxic, minic,	/* Minimium and maximum curvature.      */
      slope,			/* Slope.                               */
      crosc;			/* Cross-sectional curvature.           */

    minic = (-a - b - sqrt((a - b) * (a - b) + c * c));
    maxic = (-a - b + sqrt((a - b) * (a - b) + c * c));
    slope = RAD2DEG * atan(sqrt((d * d) + (e * e)));
    crosc = -2.0 * (b * d * d + a * e * e - c * d * e) / (d * d + e * e);


    /*
       Feature slope crosc maxic minic

       Peak    0     #     +ve   +ve
       Ridge   0     #     +ve   0
       +ve   +ve   #     #
       Pass    0     #     +ve   -ve
       Plane   0     #     0     0
       +ve   0     #     #
       Channel 0     #     0     -ve
       +ve   -ve   #     #
       Pit     0     #     -ve   -ve

       Table 5.3 Simplified feature classification criteria.
       #  indicates undefined, or not part of selection criteria.
       http://www.geog.le.ac.uk/jwo/research/dem_char/thesis/05feat.htm
     */

    /* Case 1: Surface is sloping. Cannot be a peak,pass or pit. Therefore
       calculate the cross-sectional curvature to characterise as
       channel, ridge or planar.                                   */

    if (slope > slope_tol) {
	if (crosc > curve_tol) {
	    return (RIDGE);
	}
	else if (crosc < -curve_tol) {
	    return (CHANNEL);
	}
	else {
	    return (FLAT);
	}
    }
    else {


	/* Case 2: Surface has (approximately) vertical slope normal. Feature
	   can be of any type.                                        */

	if (maxic > curve_tol) {
	    if (minic > curve_tol) {
		return (PEAK);
	    }
	    else if (minic < -curve_tol) {
		return (PASS);
	    }
	    else {
		return (RIDGE);
	    }
	}
	else if (maxic < -curve_tol) {
	    if (minic < -curve_tol) {
		return (PIT);
	    }
	}
	else {
	    if (minic < -curve_tol) {
		return (CHANNEL);
	    }
	    else if (minic > curve_tol && minic < -curve_tol) {
		return (FLAT);
	    }
	}
    }
    return (FLAT);
}
