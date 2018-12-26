
/******************************************************************************
 *	spot_dist.c 		in ~/r.ros
 *	
 *  	Returns the maximum spotting distance (in meters) based on:
 * 1) Lathrop, Richard G. and Jianping Xu, 1994, A geographic information
 *	system method to assist evaluating spotting distance in any 
 * 	terrain conditions. (in preparation) 
 *
 *	|
 *	|
 *	|***    z0 = e0 + h0	
 *	|     *
 *	|       *   firebrand trajetory z: z = z0 - (X/(1.3U))^2 
 *	|	 * 			   simplified from Chase (1984)
 *	|	  *
 *	|	   *
 *	| 	    *
 *	|\ e0	     *
 *	| \___    __  *   /\
 *	|     \__/  \__*_/  \   surface e: from DEM 	
 *	.		     \
 *	.
 *	
 *	|___________________________________ X
 *
 * 2) Chase, Carolyn, H., 1984, Spotting distance from wind-driven surface 
 *	fires -- ententions of equations for pocket calculators, US Forest 
 *	Service, Res. Note INT-346 , Ogden, Uhta, 21 p.
 * 3) Rothermel, Richard, 1991, Predicting behavior and size of crown fires 
 *	in the northern Rocky Mountains, US Forest Service, Res. Paper 
 *	INT-438, 46 p.
 ******************************************************************************/

#include <math.h>
#include <grass/raster.h>

#define DATA(map, r, c) (map)[(r) * ncols + (c)]
#define	DEG2RAD		M_D2R
/*#define DEBUG */


/* Ovendry loading for all sizes (lb/ft^2) */
float w[14] = { 0, 0.034, 0.184, 0.138, 0.736, 0.161, 0.276,
    0.224, 0.230, 0.160, 0.552, 0.529, 1.587, 2.668
};

/* Mean average cover height (assumed the same as current fuel depth (ft) */
float hbar[] = { 0, 1.0, 1.0, 2.5, 6.0, 2.0, 2.5,
    2.5, 0.2, 0.2, 1.0, 1.0, 2.3, 3.0
};

/* A Coeficients in E = IA(.474U)^B (s), where U is wind at 20 ft (mi/h) */
double A[14] = { 0, 545, 709, 429, 301, 235, 242,
    199, 0, 1121, 224, 179, 163, 170
};

/* B Coeficients in E = IA(.474U)^B (s), where U is wind at 20 ft (mi/h) */
double B[14] = { 0, -1.21, -1.32, -1.19, -1.05, -0.92, -0.94,
    0.83, 0, -1.51, -0.89, -0.81, -0.78, -0.79
};

/**
 * @brief Compute maximum spotting distance
 *
 * @param fuel fuel type used in Byram's equation from Rothermel (1991)
 *        and in Chase (1984) equation for source z
 * @param maxros maximal ROS used in Byram's equation
 * @param speed wind speed used to compute mean windspeed at 6 meter
 *        according to Chase (1984) influencing the target z
 * @param angle direction of maximal ROS, i.e. the direction of spotting
 *        (if you think that only direction of wind influences the spotting
 *        then this should be the wind direction)
 * @param row0 source cell row
 * @param col0 source cell column
 * @return maximum spotting distance
 */
int spot_dist(int fuel, float maxros, int speed, float angle, int row0,
	      int col0)
{
    /* variables in Chase (1984) (use h0 for z0 which for e0 + h0) */
    double h0;			/* initial firebrand height (m) */
    double E;			/* thermal strength (Btu/ft) */
    double I;			/* mean fire intensity (Btu/ft/s) */
    double U;			/* mean windspeed at 6 meter (km/h) */

    /* variables in Rothermel (1991) */
    float R;			/* forward rate of spread (ROS) (ft/s) */

    /* other variables */
    extern CELL *map_elev;	/* elevation map array */
    extern int nrows, ncols;
    extern struct Cell_head window;
    double z0;			/* initial firebrand elevation (m) */
    double z;			/* firebrand height (m) */
    int row, col;		/* a cell under a spotting trajatory */
    int S;			/* spotting distance on an terrain (m) */
    double sqrd;		/* distance from cell0 to cell */
    double sin_a, cos_a;	/* of the wind angle */
    double sqr_ns, sqr_ew;	/* square resolutions for speed */
    int i;			/* for advance a step */

    if (fuel == 8)		/* no spotting from closed timber litter */
	return (0);

    /* get I from Byram's equation, I = R*w*h, cited in Rothermel (1991) */
    R = maxros / 60.0;
    I = R * w[fuel] * 8000;

    /* get h0 (originally z0) and z0 (e0 + h0) from Chase (1984) */
    U = 2 * speed / 88.0;
    if (U == 0.0)
	E = 0.0;		/* to avoid domain error in pow() */
    else
	E = I * A[fuel] * pow(0.474 * U, B[fuel]);
    h0 = 0.3048 * (1.055 * sqrt(E));
    U *= 1.609;			/* change units to metric */
    z0 = DATA(map_elev, row0, col0) + h0;

    sin_a = sin(angle * DEG2RAD), cos_a = cos(angle * DEG2RAD);
    sqr_ns = window.ns_res * window.ns_res;
    sqr_ew = window.ew_res * window.ew_res;

    /* vertical change using F=1.3*U*(dz)^.5, simplified from Chase (1984) */
    S = 0;
    row = row0 - cos_a + 0.5, col = col0 + sin_a + 0.5;
    if (row < 0 || row >= nrows || col < 0 || col >= ncols)	/* outside */
	return (S);

    i = 1;
    while (1) {
	if (row < 0 || row >= nrows)	/* outside the region */
	    break;
	if (col < 0 || col >= ncols)	/* outside the region */
	    break;
	sqrd = (row - row0) * (row - row0) * sqr_ns
	    + (col - col0) * (col - col0) * sqr_ew;
	z = z0 - sqrd / (1.69 * U * U);

	/* actual target elevation is higher then the potential one */
	if (DATA(map_elev, row, col) > z) {
#ifdef DEBUG
	    printf
		("\nA return: m%d U=%d(m/h) h0=%d(m) e0(%d,%d)=%d z=%d(m) e(%d,%d)=%d s=%d(m)",
		 (int)fuel, (int)U, (int)h0, row0, col0, DATA(map_elev, row0,
							      col0), (int)z,
		 row, col, DATA(map_elev, row, col), S);
#endif
	    return (S);
	}
	/* advance a step, increase the spotting distance */
	S = sqrt((double)sqrd);
#ifdef DEBUG
	printf
	    ("\nm%d U=%d(m/h) h0=%d(m) e0(%d,%d)=%d z=%d(m) e(%d,%d)=%d s=%d(m)",
	     (int)fuel, (int)U, (int)h0, row0, col0, DATA(map_elev, row0,
							  col0), (int)z, row,
	     col, DATA(map_elev, row, col), S);
#endif
	i++;
	row = row0 - i * cos_a + 0.5, col = col0 + i * sin_a + 0.5;
	if (row < 0 || row >= nrows || col < 0 || col >= ncols)
	    return (S);		/* outside the region */
    }

    return S;
}
