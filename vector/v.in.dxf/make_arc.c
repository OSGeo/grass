#include <math.h>
#include "global.h"

#define RSTEP 5.0
#define DEG_TO_RAD (M_PI/180.0)

int make_arc(int offset,	/* offset into array of points */
	     double centerx, double centery,
	     double radius, double start_angle, double finish_angle,
	     double zcoor)
{
    float theta;		/* the angle used for calculating a given point */
    float alpha;		/* theta converted into radians for use in math */
    int arr_size;

    arr_size = offset;
    G_debug(3,
	    "making arc: offset %d  x %.1f y %.1f rad %.1f a1 %.1f a2 %.1f\n",
	    offset, centerx, centery, radius, start_angle, finish_angle);
    if (start_angle > finish_angle)
	finish_angle = 360. + finish_angle;
    /* negative radius indicates that arc is to be drawn in a clockwise
     * direction from start_angle to finish_angle
     */
    if (radius < 0) {
	start_angle = 360. + start_angle;
	theta = start_angle;
	radius = -radius;
	while (theta > finish_angle) {
	    alpha = theta * DEG_TO_RAD;	/* converting to radians */
	    xpnts[arr_size] = radius * cos(alpha) + centerx;
	    ypnts[arr_size] = radius * sin(alpha) + centery;
	    zpnts[arr_size] = zcoor;
	    arr_size++;
	    theta -= RSTEP;
	    if (arr_size == arr_max) {
		arr_max += ARR_INCR;
		xpnts = (double *)G_realloc(xpnts, arr_max * sizeof(double));
		ypnts = (double *)G_realloc(ypnts, arr_max * sizeof(double));
		zpnts = (double *)G_realloc(zpnts, arr_max * sizeof(double));
	    }
	}
    }
    else {
	theta = start_angle;
	while (theta < finish_angle) {	/* draw arc counterclockwise */
	    alpha = theta * DEG_TO_RAD;	/* converting to radians */
	    xpnts[arr_size] = radius * cos(alpha) + centerx;
	    ypnts[arr_size] = radius * sin(alpha) + centery;
	    zpnts[arr_size] = zcoor;
	    arr_size++;
	    theta += RSTEP;
	    if (arr_size == arr_max) {
		arr_max += ARR_INCR;
		xpnts = (double *)G_realloc(xpnts, arr_max * sizeof(double));
		ypnts = (double *)G_realloc(ypnts, arr_max * sizeof(double));
		zpnts = (double *)G_realloc(zpnts, arr_max * sizeof(double));
	    }
	}
    }
    /* this insures that the last point will be correct */
    alpha = finish_angle * DEG_TO_RAD;	/* converting to radians */
    xpnts[arr_size] = radius * cos(alpha) + centerx;
    ypnts[arr_size] = radius * sin(alpha) + centery;
    zpnts[arr_size] = zcoor;
    arr_size++;
    if (arr_size == arr_max) {
	arr_max += ARR_INCR;
	xpnts = (double *)G_realloc(xpnts, arr_max * sizeof(double));
	ypnts = (double *)G_realloc(ypnts, arr_max * sizeof(double));
	zpnts = (double *)G_realloc(zpnts, arr_max * sizeof(double));
    }

    return arr_size - offset;
}

int make_arc_from_polyline(int arr_size, double bulge, double prev_bulge)
{
    int arc_arr_size;
    float ang1, ang2;
    double x1, x2, y1, y2, cent_x, cent_y, rad, beta, half_alpha;
    double arc_tan = 0.0;

    /* if prev segment is an arc (prev_bulge != 0) prepare to make arc */
    if (prev_bulge > 0.0)
	arc_tan = prev_bulge;
    else if (prev_bulge < 0.0)
	arc_tan = (-1.0) * prev_bulge;

    if (arc_tan == 0.0) {	/* straight line segment */
	arr_size++;
	if (arr_size == arr_max) {
	    arr_max += ARR_INCR;
	    xpnts = (double *)G_realloc(xpnts, arr_max * sizeof(double));
	    ypnts = (double *)G_realloc(ypnts, arr_max * sizeof(double));
	    zpnts = (double *)G_realloc(zpnts, arr_max * sizeof(double));
	}
    }
    else if (!(xpnts[arr_size - 1] == xpnts[arr_size] &&
	       ypnts[arr_size - 1] == ypnts[arr_size]))
	/* make an arc */
    {
	/* compute cent_x, cent_y, ang1, ang2 */
	if (prev_bulge > 0.0) {
	    x1 = xpnts[arr_size - 1];
	    x2 = xpnts[arr_size];
	    y1 = ypnts[arr_size - 1];
	    y2 = ypnts[arr_size];
	}
	else {
	    /* figure out how to compute the opposite center */
	    x2 = xpnts[arr_size - 1];
	    x1 = xpnts[arr_size];
	    y2 = ypnts[arr_size - 1];
	    y1 = ypnts[arr_size];
	}
	half_alpha = (double)atan(arc_tan) * 2.;
	rad = hypot(x1 - x2, y1 - y2) * .5 / sin(half_alpha);
	beta = atan2(x1 - x2, y1 - y2);
	/* now bring it into range 0 to 360 */
	beta = 90.0 * DEG_TO_RAD - beta;
	if (beta <= 0.0)
	    beta = 360.0 * DEG_TO_RAD + beta;
	/* now beta is counter clock wise from 0 (direction of (1,0)) to 360 */
	if (beta >= 0.0 && beta < 90.0) {
	    cent_x = x2 + rad * sin(half_alpha + beta);
	    cent_y = y2 - rad * cos(half_alpha + beta);
	    ang2 = (half_alpha + beta) / DEG_TO_RAD + 90.0;
	    ang1 = (beta - half_alpha) / DEG_TO_RAD + 90.0;
	}
	else if (beta >= 90.0 && beta < 180.0) {
	    beta -= 90.0;
	    cent_y = y2 + rad * sin(half_alpha + beta);
	    cent_x = x2 + rad * cos(half_alpha + beta);
	    ang2 = (half_alpha + beta) / DEG_TO_RAD + 180.0;
	    ang1 = (beta - half_alpha) / DEG_TO_RAD + 180.0;
	}
	else if (beta >= 180.0 && beta < 270.0) {
	    beta -= 180.0;
	    cent_x = x2 - rad * sin(half_alpha + beta);
	    cent_y = y2 + rad * cos(half_alpha + beta);
	    ang2 = (half_alpha + beta) / DEG_TO_RAD + 270.0;
	    ang1 = (beta - half_alpha) / DEG_TO_RAD + 270.0;
	}
	else {			/* 270 <= beta < 360 */

	    beta -= 270.0;
	    cent_y = y2 - rad * sin(half_alpha + beta);
	    cent_x = x2 - rad * cos(half_alpha + beta);
	    ang2 = (half_alpha + beta) / DEG_TO_RAD;
	    ang1 = (beta - half_alpha) / DEG_TO_RAD;
	}

	arr_size--;		/* disregard last 2 points */
	if (prev_bulge < 0.0)
	    arc_arr_size = make_arc(arr_size, cent_x, cent_y,
				    -rad, ang2, ang1, zpnts[0]);
	/* arc is going in clockwise direction from x2 to x1 */
	else

	    arc_arr_size = make_arc(arr_size, cent_x, cent_y,
				    rad, ang1, ang2, zpnts[0]);
	arr_size += arc_arr_size;
	while (arr_size >= arr_max) {
	    arr_max += ARR_INCR;
	    xpnts = (double *)G_realloc(xpnts, arr_max * sizeof(double));
	    ypnts = (double *)G_realloc(ypnts, arr_max * sizeof(double));
	    zpnts = (double *)G_realloc(zpnts, arr_max * sizeof(double));
	}
    }				/* arc */

    return arr_size;
}
