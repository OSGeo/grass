/*
 * - Stefano Menegon/Lorenzo Potrich: curvatures fixed Jan 2002 
 * - FP update Lorenzo Potrich/Markus Neteler Jan 2002
 * - Changes line 59 for Linux - Markus Neteler Jan 1998 
 */

/*****************************************************************************/
/***                                                                       ***/
/***                                param()                                ***/
/***     Returns a terrain parameter based on the 6 quadratic coefficients  ***/
/***	 that define a local trend surface. 			    	   ***/
/***     Jo Wood, Department of Geography, V2.0 15th December, 1994        ***/
/***                                                                       ***/
/*****************************************************************************/

#include "param.h"
#include <math.h>


DCELL param(int ptype,		/* Type of terrain parameter to calculate */
	    double *coeff)
{				/* Set of six quadratic coefficients.        */

    /* Quadratic function in the form of

       z = ax^2 + by^2 + cxy + dx + ey +f                         */

    double a = C_A * zscale,	/* Rescale coefficients if a      */
	b = C_B * zscale,	/* Z scaling is required.         */
	c = C_C * zscale, d = C_D * zscale, e = C_E * zscale, f = C_F;	/* f does not need rescaling as   */

    /* it is only used for smoothing. */

    switch (ptype) {
    case ELEV:
	return (f);
	break;

    case SLOPE:
	return (atan(sqrt(d * d + e * e)) * RAD2DEG);
	break;

    case ASPECT:
	return (atan2(e, d) * RAD2DEG);
	break;

    case PROFC:
	if ((d == 0) && (e == 0))
	    return (0.0);
	else
	    return (-2.0 * (a * d * d + b * e * e + c * e * d) /
		    ((e * e + d * d) * pow(1.0 + d * d + e * e, 1.5)));
	break;

    case PLANC:
	if ((d == 0) && (e == 0))
	    return (0.0);
	else
	    return (2.0 * (b * d * d + a * e * e - c * d * e) /
		    pow(e * e + d * d, 1.5));
	break;

    case LONGC:
	if ((d == 0) && (e == 0))
	    return (0.0);
	else
	    return (-2.0 * (a * d * d + b * e * e + c * d * e) /
		    (d * d + e * e));
    case CROSC:
	if ((d == 0) && (e == 0))
	    return (0.0);
	else
	    return (-2.0 * (b * d * d + a * e * e - c * d * e) /
		    (d * d + e * e));

    case MINIC:
	return (-a - b - sqrt((a - b) * (a - b) + c * c));

    case MAXIC:
	return (-a - b + sqrt((a - b) * (a - b) + c * c));

    default:
	return (0.0);
    }
}
