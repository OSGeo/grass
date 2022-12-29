
/*!
 * \file lib/gis/wind_overlap.c
 *
 * \brief GIS Library - Window overlap functions.
 *
 * (C) 2001-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2014
 */

#include <grass/gis.h>


/**
 * \brief Determines if a box overlays a map window.
 *
 * Given a map <b>window</b>, and a box of <b>N</b>,<b>S</b>,<b>E</b>,<b>W</b>
 * does the box overlap the map <b>window</b>?<br>
 *
 * Note: knows about global wrap-around for lat-long.
 *
 * \param[in] window pointer to window structure
 * \param[in] N north
 * \param[in] S south
 * \param[in] E east
 * \param[in] W west
 * \return 1 if box overlaps window
 * \return 0 if box does not overlap window
 */

int G_window_overlap(const struct Cell_head *window,
		     double N, double S, double E, double W)
{
    if (window->north <= S)
	return 0;
    if (window->south >= N)
	return 0;

    if (window->proj == PROJECTION_LL) {
	while (E < window->west) {
	    E += 360.0;
	    W += 360.0;
	}
	while (W > window->east) {
	    E -= 360.0;
	    W -= 360.0;
	}
    }

    if (window->east <= W)
	return 0;
    if (window->west >= E)
	return 0;

    return 1;
}


/**
 * \brief Determines percentage of box is contained in the <b>window</b>.
 *
 * This version returns the percentage (from 0 to 1) of the box 
 * contained in the window. This feature can be used during vector
 * plotting to decide if it is more efficient to do a level-one
 * read of the whole vector map, or to pay the price of a
 * level-two startup so only those arcs that enter the window are
 * actually read.
 *
 * \param[in] window pointer to widnow structure
 * \param[in] N north
 * \param[in] S south
 * \param[in] E east
 * \param[in] W west
 * \return percentage of overlap
 */

double G_window_percentage_overlap(const struct Cell_head *window,
				   double N, double S, double E, double W)
{
    double V, H;
    double n, s, e, w;
    double shift;

    /* vertical height of the box that overlaps the window */
    if ((n = window->north) > N)
	n = N;
    if ((s = window->south) < S)
	s = S;
    V = n - s;

    if (N == S) {
	V = (N < window->north && N > window->south);
	N = 1;
	S = 0;
    }

    if (V <= 0.0)
	return 0.0;

    /* global wrap-around, part 1 */
    if (window->proj == PROJECTION_LL) {
	shift = 0.0;
	while (E + shift > window->east)
	    shift -= 360.0;
	while (E + shift < window->west)
	    shift += 360.0;
	E += shift;
	W += shift;
    }

    /* horizontal width of the box that overlaps the window */
    if ((e = window->east) > E)
	e = E;
    if ((w = window->west) < W)
	w = W;
    H = e - w;
    if (W == E)
	H = (E > window->west && E < window->east);
    if (H <= 0.0)
	return 0.0;

    /* global wrap-around, part 2 */
    if (window->proj == PROJECTION_LL) {
	shift = 0.0;
	while (W + shift < window->west)
	    shift += 360.0;
	while (W + shift > window->east)
	    shift -= 360.0;
	if (shift) {
	    E += shift;
	    W += shift;
	    if ((e = window->east) > E)
		e = E;
	    if ((w = window->west) < W)
		w = W;
	    H += e - w;
	}
    }
    if (W == E) {
	W = 0;
	E = 1;
    }

    return (H * V) / ((N - S) * (E - W));
}
