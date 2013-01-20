/*!
 * \file gis/area.c
 *
 * \brief GIS Library - Area calculation functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>

static struct state {
    struct Cell_head window;
    double square_meters;
    int projection;

    double units_to_meters_squared;

    /* these next are for lat-long only */
    int next_row;
    double north_value;
    double north;
    double (*darea0) (double);
} state;

static struct state *st = &state;

/*!
 * \brief Begin cell area calculations.
 *
 * This routine must be called once before any call to
 * G_area_of_cell_at_row(). It perform all inititalizations needed to
 * do area calculations for grid cells, based on the current window
 * "projection" field. It can be used in either planimetric
 * projections or the latitude-longitude projection.
 *
 * \return 0 if the projection is not measurable (ie. imagery or xy)
 * \return 1 if the projection is planimetric (ie. UTM or SP)
 * \return 2 if the projection is non-planimetric (ie. latitude-longitude)
 */

int G_begin_cell_area_calculations(void)
{
    double a, e2;
    double factor;

    G_get_set_window(&st->window);
    switch (st->projection = st->window.proj) {
    case PROJECTION_LL:
	G_get_ellipsoid_parameters(&a, &e2);
	if (e2) {
	    G_begin_zone_area_on_ellipsoid(a, e2, st->window.ew_res / 360.0);
	    st->darea0 = G_darea0_on_ellipsoid;
	}
	else {
	    G_begin_zone_area_on_sphere(a, st->window.ew_res / 360.0);
	    st->darea0 = G_darea0_on_sphere;
	}
	st->next_row = 0;
	st->north = st->window.north;
	st->north_value = st->darea0(st->north);
	return 2;
    default:
	st->square_meters = st->window.ns_res * st->window.ew_res;
	factor = G_database_units_to_meters_factor();
	if (factor > 0.0)
	    st->square_meters *= (factor * factor);
	return (factor > 0.0);
    }
}

/*!
 * \brief Cell area in specified row.
 *
 * This routine returns the area in square meters of a cell in the
 * specified <i>row</i>. This value is constant for planimetric grids 
 * and varies with the row if the projection is latitude-longitude.
 *
 * \param row row number
 *
 * \return cell area
 */
double G_area_of_cell_at_row(int row)
{
    double south_value;
    double cell_area;

    if (st->projection != PROJECTION_LL)
	return st->square_meters;

    if (row != st->next_row) {
	st->north = st->window.north - row * st->window.ns_res;
	st->north_value = st->darea0(st->north);
    }

    st->north -= st->window.ns_res;
    south_value = st->darea0(st->north);
    cell_area = st->north_value - south_value;

    st->next_row = row + 1;
    st->north_value = south_value;

    return cell_area;
}

/*!
 * \brief Begin polygon area calculations.
 *
 * This initializes the polygon area calculation routines. It is used
 * both for planimetric and latitude-longitude projections.
 *
 * \return 0 if the projection is not measurable (ie. imagery or xy)
 * \return 1 if the projection is planimetric (ie. UTM or SP)
 * \return 2 if the projection is non-planimetric (ie. latitude-longitude)
 */
int G_begin_polygon_area_calculations(void)
{
    double a, e2;
    double factor;

    if ((st->projection = G_projection()) == PROJECTION_LL) {
	G_get_ellipsoid_parameters(&a, &e2);
	G_begin_ellipsoid_polygon_area(a, e2);
	return 2;
    }
    factor = G_database_units_to_meters_factor();
    if (factor > 0.0) {
	st->units_to_meters_squared = factor * factor;
	return 1;
    }
    st->units_to_meters_squared = 1.0;
    return 0;
}

/*!
 * \brief Area in square meters of polygon.
 *
 * Returns the area in square meters of the polygon described by the 
 * <i>n</i> pairs of <i>x,y</i> coordinate vertices. It is used both for 
 * planimetric and latitude-longitude projections.
 * 
 * Returns the area in coordinate units of the polygon described by
 * the <i>n</i> pairs of <i>x,y</i> coordinate vertices for
 * planimetric grids.  If the units for <i>x,y</i> are meters, then
 * the area is in square meters.  If the units are feet, then the area
 * is in square feet, and so on.
 *
 * You should call G_begin_polygon_area_calculations() function before
 * calling this function.
 *
 * <b>Note:</b> If the database is planimetric with the non-meter grid, 
 * this routine performs the required unit conversion to produce square 
 * meters.
 *
 * \param x array of x coordinates
 * \param y array of y coordinates
 * \param n number of x,y coordinate pairs
 *
 * \return area in coordinate units of the polygon
 */
double G_area_of_polygon(const double *x, const double *y, int n)
{
    double area;

    if (st->projection == PROJECTION_LL)
	area = G_ellipsoid_polygon_area(x, y, n);
    else
	area = G_planimetric_polygon_area(x, y, n) * st->units_to_meters_squared;

    return area;
}
