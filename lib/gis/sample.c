/*!
   \file sample.c

   \brief GIS library - sampling methods (extract a cell value from
   raster map)

   1/2006: moved to libgis from v.sample/v.drape for clone removal

   (C) 2001-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author James Darrell McCauley <darrell mccauley-usa.com>, http://mccauley-usa.com/

   \date 1994
 */

#include <string.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/* prototypes */
static double scancatlabel(const char *);
static void raster_row_error(const struct Cell_head *window, double north,
			     double east);


/*!
 *  \brief Extract a cell value from raster map.
 *
 *  Extract a cell value from raster map at given northing and easting
 *  with a sampled 3x3 window using a specified interpolation method.
 *
 *  \param fd file descriptor
 *  \param window region settings
 *  \param cats categories
 *  \param north northing position
 *  \param east easting position
 *  \param usedesc flag to scan category label
 *  \param itype interpolation method
 *
 *  \return cell value at given position
 */

DCELL G_get_raster_sample(
    int fd,
    const struct Cell_head *window,
    struct Categories *cats,
    double north, double east,
    int usedesc, INTERP_TYPE itype)
{
    double retval;

    switch (itype) {
    case NEAREST:
	retval = G_get_raster_sample_nearest(fd, window, cats, north, east, usedesc);
	break;
    case BILINEAR:
	retval = G_get_raster_sample_bilinear(fd, window, cats, north, east, usedesc);
	break;
    case CUBIC:
	retval = G_get_raster_sample_cubic(fd, window, cats, north, east, usedesc);
	break;
    default:
	G_fatal_error("G_get_raster_sample: %s",
		      _("Unknown interpolation type"));
    }

    return retval;
}


DCELL G_get_raster_sample_nearest(
    int fd,
    const struct Cell_head *window,
    struct Categories *cats,
    double north, double east, int usedesc)
{
    int row, col;
    DCELL result;
    DCELL *maprow = G_allocate_d_raster_buf();

    /* convert northing and easting to row and col, resp */
    row = (int)floor(G_northing_to_row(north, window));
    col = (int)floor(G_easting_to_col(east, window));

    if (row < 0 || row >= G_window_rows() ||
	col < 0 || col >= G_window_cols()) {
	G_set_d_null_value(&result, 1);
	goto done;
    }

    if (G_get_d_raster_row(fd, maprow, row) < 0)
	raster_row_error(window, north, east);

    if (G_is_d_null_value(&maprow[col])) {
	G_set_d_null_value(&result, 1);
	goto done;
    }

    if (usedesc) {
	char *buf = G_get_cat(maprow[col], cats);

	G_squeeze(buf);
	result = scancatlabel(buf);
    }
    else
	result = maprow[col];

done:
    G_free(maprow);

    return result;
}


DCELL G_get_raster_sample_bilinear(
    int fd,
    const struct Cell_head *window,
    struct Categories *cats,
    double north, double east, int usedesc)
{
    int row, col;
    double grid[2][2];
    DCELL *arow = G_allocate_d_raster_buf();
    DCELL *brow = G_allocate_d_raster_buf();
    double frow, fcol, trow, tcol;
    DCELL result;

    frow = G_northing_to_row(north, window);
    fcol = G_easting_to_col(east, window);

    /* convert northing and easting to row and col, resp */
    row = (int)floor(frow - 0.5);
    col = (int)floor(fcol - 0.5);

    trow = frow - row - 0.5;
    tcol = fcol - col - 0.5;

    if (row < 0 || row + 1 >= G_window_rows() ||
	col < 0 || col + 1 >= G_window_cols()) {
	G_set_d_null_value(&result, 1);
	goto done;
    }

    if (G_get_d_raster_row(fd, arow, row) < 0)
	raster_row_error(window, north, east);
    if (G_get_d_raster_row(fd, brow, row + 1) < 0)
	raster_row_error(window, north, east);

    if (G_is_d_null_value(&arow[col]) || G_is_d_null_value(&arow[col + 1]) ||
	G_is_d_null_value(&brow[col]) || G_is_d_null_value(&brow[col + 1])) {
	G_set_d_null_value(&result, 1);
	goto done;
    }

    /*-
     * now were ready to do bilinear interpolation over
     * arow[col], arow[col+1],
     * brow[col], brow[col+1]
     */

    if (usedesc) {
	char *buf;

	G_squeeze(buf = G_get_cat((int)arow[col], cats));
	grid[0][0] = scancatlabel(buf);
	G_squeeze(buf = G_get_cat((int)arow[col + 1], cats));
	grid[0][1] = scancatlabel(buf);
	G_squeeze(buf = G_get_cat((int)brow[col], cats));
	grid[1][0] = scancatlabel(buf);
	G_squeeze(buf = G_get_cat((int)brow[col + 1], cats));
	grid[1][1] = scancatlabel(buf);
    }
    else {
	grid[0][0] = arow[col];
	grid[0][1] = arow[col + 1];
	grid[1][0] = brow[col];
	grid[1][1] = brow[col + 1];
    }

    result = G_interp_bilinear(tcol, trow,
			       grid[0][0], grid[0][1], grid[1][0], grid[1][1]);

done:
    G_free(arow);
    G_free(brow);

    return result;
}

DCELL G_get_raster_sample_cubic(
    int fd,
    const struct Cell_head *window,
    struct Categories *cats,
    double north, double east, int usedesc)
{
    int i, j, row, col;
    double grid[4][4];
    DCELL *rows[4];
    double frow, fcol, trow, tcol;
    DCELL result;

    for (i = 0; i < 4; i++)
	rows[i] = G_allocate_d_raster_buf();

    frow = G_northing_to_row(north, window);
    fcol = G_easting_to_col(east, window);

    /* convert northing and easting to row and col, resp */
    row = (int)floor(frow - 1.5);
    col = (int)floor(fcol - 1.5);

    trow = frow - row - 1.5;
    tcol = fcol - col - 1.5;

    if (row < 0 || row + 3 >= G_window_rows() ||
	col < 0 || col + 3 >= G_window_cols()) {
	G_set_d_null_value(&result, 1);
	goto done;
    }

    for (i = 0; i < 4; i++)
	if (G_get_d_raster_row(fd, rows[i], row + i) < 0)
	    raster_row_error(window, north, east);

    for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++)
	    if (G_is_d_null_value(&rows[i][col + j])) {
		G_set_d_null_value(&result, 1);
		goto done;
	    }

    /*
     * now were ready to do cubic interpolation over
     * arow[col], arow[col+1], arow[col+2], arow[col+3],
     * brow[col], brow[col+1], brow[col+2], brow[col+3],
     * crow[col], crow[col+1], crow[col+2], crow[col+3],
     * drow[col], drow[col+1], drow[col+2], drow[col+3],
     */

    if (usedesc) {
	char *buf;

	for (i = 0; i < 4; i++) {
	    for (j = 0; j < 4; j++) {
		G_squeeze(buf = G_get_cat(rows[i][col + j], cats));
		grid[i][j] = scancatlabel(buf);
	    }
	}
    }
    else {
	for (i = 0; i < 4; i++)
	    for (j = 0; j < 4; j++)
		grid[i][j] = rows[i][col + j];
    }

    result = G_interp_bicubic(tcol, trow,
			      grid[0][0], grid[0][1], grid[0][2], grid[0][3],
			      grid[1][0], grid[1][1], grid[1][2], grid[1][3],
			      grid[2][0], grid[2][1], grid[2][2], grid[2][3],
			      grid[3][0], grid[3][1], grid[3][2], grid[3][3]);

done:
    for (i = 0; i < 4; i++)
	G_free(rows[i]);

    return result;
}


static double scancatlabel(const char *str)
{
    double val;

    if (strcmp(str, "no data") != 0)
	sscanf(str, "%lf", &val);
    else {
	G_warning(_("\"no data\" label found; setting to zero"));
	val = 0.0;
    }

    return val;
}


static void raster_row_error(const struct Cell_head *window, double north,
			     double east)
{
    G_debug(3, "DIAG: \tRegion is: n=%g s=%g e=%g w=%g",
	    window->north, window->south, window->east, window->west);
    G_debug(3, "      \tData point is north=%g east=%g", north, east);

    G_fatal_error(_("Problem reading raster map"));
}
