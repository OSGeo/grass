
/****************************************************************
 *
 * MODULE:     v.edit
 *
 * PURPOSE:    Editing vector map.
 *
 * AUTHOR(S):  GRASS Development Team
 *             Wolf Bergenheim, Jachym Cepicky, Martin Landa
 *
 * COPYRIGHT:  (C) 2006-2008 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 * TODO:       3D support
 ****************************************************************/

#include "global.h"

/**
   \brief Set distance based on the current resolution

   This code comes from v.what/main.c
 
   \param[in] maxdistance max distance

   \return result max distance
*/
double max_distance(double maxdistance)
{
    struct Cell_head window;

    double ew_dist1, ew_dist2, ns_dist1, ns_dist2;
    double xres, yres, maxd;

    if (maxdistance < 0.0) {
	G_get_window(&window);

	G_begin_distance_calculations();

	ew_dist1 =
	    G_distance(window.east, window.north, window.west, window.north);
	/* EW Dist at South Edge */
	ew_dist2 =
	    G_distance(window.east, window.south, window.west, window.south);
	/* NS Dist at East edge */
	ns_dist1 =
	    G_distance(window.east, window.north, window.east, window.south);
	/* NS Dist at West edge */
	ns_dist2 =
	    G_distance(window.west, window.north, window.west, window.south);

	xres = ((ew_dist1 + ew_dist2) / 2) / window.cols;
	yres = ((ns_dist1 + ns_dist2) / 2) / window.rows;

	if (xres > yres)
	    maxd = xres;
	else
	    maxd = yres;

	/*
	   G_important_message (_("Threshold distance set to %g map units (based on 2D resolution)"), maxd);
	 */
    }
    else {
	maxd = maxdistance;
    }

    G_debug(3, "max_distance(): threshold is %g", maxd);

    return maxd;
}

/**
   \brief Creates bounding box (polygon)

   Based on center point; size (2 * maxdist)

   \param[in] east,north coordinates of center
   \param[in] maxdist size of bounding box
   \param[out] result bounding box

   \return
*/
void coord2bbox(double east, double north, double maxdist,
		struct line_pnts *box)
{
    /* TODO: 3D */
    Vect_reset_line(box);

    Vect_append_point(box, east - maxdist, north - maxdist, 0);
    Vect_append_point(box, east + maxdist, north - maxdist, 0);
    Vect_append_point(box, east + maxdist, north + maxdist, 0);
    Vect_append_point(box, east - maxdist, north + maxdist, 0);
    Vect_append_point(box, box->x[0], box->y[0], box->z[0]);

    return;
}
