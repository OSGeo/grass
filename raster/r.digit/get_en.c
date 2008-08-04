
/****************************************************************************
 *
 * MODULE:       r.digit
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Interactive tool used to draw and save vector features
 *               on a graphics monitor using a pointing device (mouse)
 *               and save to a raster map.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <grass/gis.h>


int get_east_north(int x, int y, char *east, char *north)
{
    double e, n;

    G_plot_where_en(x, y, &e, &n);
    G_format_easting(e, east, G_projection());
    G_format_northing(n, north, G_projection());

    return 0;
}
