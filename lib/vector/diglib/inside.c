/*
 ****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Original author CERL, probably Dave Gerdes.
 *               Update to GRASS 5.7 Radim Blazek.
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
#include <grass/vector.h>

double
dig_x_intersect(double beg_x,
		double end_x, double beg_y, double end_y, double Y)
{
    double b, a;

    b = (end_x - beg_x) / (end_y - beg_y);
    a = beg_x - b * beg_y;
    return (a + b * Y);
}

int dig_in_area_bbox(struct P_area * Area, double x, double y)
{
#ifdef GDEBUG
    G_debug(3, "BBOX: (x,y) (%lf, %lf)\n", x, y);
    G_debug(3, "NSEW:  %lf, %lf, %lf, %lf\n", Area->N, Area->S, Area->E,
	    Area->W);
#endif
    if (x < Area->W)
	return (0);
    if (x > Area->E)
	return (0);
    if (y < Area->S)
	return (0);
    if (y > Area->N)
	return (0);

    return (1);
}
