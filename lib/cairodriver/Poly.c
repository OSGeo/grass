/*!
  \file cairodriver/Poly.c

  \brief GRASS cairo display driver - draw polygon/polyline

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include "cairodriver.h"

static void do_polygon(const double *xarray, const double *yarray, int count)
{
    int i;

    cairo_move_to(cairo, xarray[0], yarray[0]);
    for (i = 1; i < count; i++)
	cairo_line_to(cairo, xarray[i], yarray[i]);
}

/*!
  \brief Draw polygon (filled)

  \param xarray,yarray array of x/y coordinates
  \param count number of points
*/
void Cairo_Polygon(const double *xarray, const double *yarray, int count)
{
    G_debug(3, "Cairo_Polygon (%d points)", count);
    do_polygon(xarray, yarray, count);
    cairo_fill(cairo);
    ca.modified = 1;

    return;
}

/*!
  \brief Draw polyline

  \param xarray,yarray array of x/y coordinates
  \param count number of points
*/
void Cairo_Polyline(const double *xarray, const double *yarray, int count)
{
    G_debug(3, "Cairo_Polyline (%d points)", count);
    do_polygon(xarray, yarray, count);
    cairo_stroke(cairo);
    ca.modified = 1;

    return;
}

/*!
  \brief Draw polyline points

  \param xarray,yarray array of x/y coordinates
  \param count number of points
*/
void Cairo_Polydots(const double *xarray, const double *yarray, int count)
{
    int i;

    G_debug(3, "Cairo_Polydots (%d points)", count);
    for (i = 1; i < count; i++)
	Cairo_draw_point(xarray[0], yarray[0]);
    ca.modified = 1;

    return;
}
