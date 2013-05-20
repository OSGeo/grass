/*!
   \file raster/sample.c

   \brief Raster library - Sampling methods (extract a cell value from
   raster map)

   1/2006: moved to libgis from v.sample/v.drape for clone removal

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author James Darrell McCauley <darrell mccauley-usa.com>, http://mccauley-usa.com/
 */

#include <string.h>
#include <unistd.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/* prototypes */
static double scancatlabel(const char *);

/*!
 *  \brief Extract a cell value from raster map.
 *
 *  Extract a cell value from raster map at given northing and easting
 *  with a sampled 3x3 window using a specified interpolation method.
 *
 *  - NEAREST  neighbor interpolation
 *  - BILINEAR bilinear interpolation
 *  - CUBIC    cubic interpolation
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
DCELL Rast_get_sample(int fd,
		      const struct Cell_head *window,
		      struct Categories *cats,
		      double north, double east,
		      int usedesc, INTERP_TYPE itype)
{
    double retval;

    switch (itype) {
    case INTERP_NEAREST:
	retval =
	    Rast_get_sample_nearest(fd, window, cats, north, east, usedesc);
	break;
    case INTERP_LINEAR:
	retval =
	    Rast_get_sample_bilinear(fd, window, cats, north, east, usedesc);
	break;
    case INTERP_CUBIC:
	retval =
	    Rast_get_sample_cubic(fd, window, cats, north, east, usedesc);
	break;
    default:
	G_fatal_error("Rast_get_sample: %s", _("Unknown interpolation type"));
    }

    return retval;
}

/*!
 *  \brief Extract a cell value from raster map (neighbor interpolation)
 *
 *  Extract a cell value from raster map at given northing and easting
 *  with a sampled 3x3 window using a neighbor interpolation.
 *
 *  \param fd file descriptor
 *  \param window region settings
 *  \param cats categories
 *  \param north northing position
 *  \param east easting position
 *  \param usedesc flag to scan category label
 *
 *  \return cell value at given position
 */
DCELL Rast_get_sample_nearest(int fd,
			      const struct Cell_head * window,
			      struct Categories * cats,
			      double north, double east, int usedesc)
{
    int row, col;
    DCELL result;
    DCELL *maprow = Rast_allocate_d_buf();

    /* convert northing and easting to row and col, resp */
    row = (int)floor(Rast_northing_to_row(north, window));
    col = (int)floor(Rast_easting_to_col(east, window));

    if (row < 0 || row >= Rast_window_rows() ||
	col < 0 || col >= Rast_window_cols()) {
	Rast_set_d_null_value(&result, 1);
	goto done;
    }

    Rast_get_d_row(fd, maprow, row);

    if (Rast_is_d_null_value(&maprow[col])) {
	Rast_set_d_null_value(&result, 1);
	goto done;
    }

    if (usedesc) {
	char *buf = Rast_get_c_cat((CELL *) & (maprow[col]), cats);

	G_squeeze(buf);
	result = scancatlabel(buf);
    }
    else
	result = maprow[col];

  done:
    G_free(maprow);

    return result;
}


/*!
 *  \brief Extract a cell value from raster map (bilinear interpolation).
 *
 *  Extract a cell value from raster map at given northing and easting
 *  with a sampled 3x3 window using a bilinear interpolation.
 *
 *  \param fd file descriptor
 *  \param window region settings
 *  \param cats categories
 *  \param north northing position
 *  \param east easting position
 *  \param usedesc flag to scan category label
 *
 *  \return cell value at given position
 */
DCELL Rast_get_sample_bilinear(int fd,
			       const struct Cell_head * window,
			       struct Categories * cats,
			       double north, double east, int usedesc)
{
    int row, col;
    double grid[2][2];
    DCELL *arow = Rast_allocate_d_buf();
    DCELL *brow = Rast_allocate_d_buf();
    double frow, fcol, trow, tcol;
    DCELL result;

    frow = Rast_northing_to_row(north, window);
    fcol = Rast_easting_to_col(east, window);

    /* convert northing and easting to row and col, resp */
    row = (int)floor(frow - 0.5);
    col = (int)floor(fcol - 0.5);

    trow = frow - row - 0.5;
    tcol = fcol - col - 0.5;

    if (row < 0 || row + 1 >= Rast_window_rows() ||
	col < 0 || col + 1 >= Rast_window_cols()) {
	Rast_set_d_null_value(&result, 1);
	goto done;
    }

    Rast_get_d_row(fd, arow, row);
    Rast_get_d_row(fd, brow, row + 1);

    if (Rast_is_d_null_value(&arow[col]) ||
	Rast_is_d_null_value(&arow[col + 1]) ||
	Rast_is_d_null_value(&brow[col]) ||
	Rast_is_d_null_value(&brow[col + 1])) {
	Rast_set_d_null_value(&result, 1);
	goto done;
    }

    /*-
     * now were ready to do bilinear interpolation over
     * arow[col], arow[col+1],
     * brow[col], brow[col+1]
     */

    if (usedesc) {
	char *buf;

	G_squeeze(buf = Rast_get_c_cat((int *)&(arow[col]), cats));
	grid[0][0] = scancatlabel(buf);
	G_squeeze(buf = Rast_get_c_cat((CELL *) & (arow[col + 1]), cats));
	grid[0][1] = scancatlabel(buf);
	G_squeeze(buf = Rast_get_c_cat((CELL *) & (brow[col]), cats));
	grid[1][0] = scancatlabel(buf);
	G_squeeze(buf = Rast_get_c_cat((CELL *) & (brow[col + 1]), cats));
	grid[1][1] = scancatlabel(buf);
    }
    else {
	grid[0][0] = arow[col];
	grid[0][1] = arow[col + 1];
	grid[1][0] = brow[col];
	grid[1][1] = brow[col + 1];
    }

    result = Rast_interp_bilinear(tcol, trow,
				  grid[0][0], grid[0][1], grid[1][0],
				  grid[1][1]);

  done:
    G_free(arow);
    G_free(brow);

    return result;
}

/*!
 *  \brief Extract a cell value from raster map (cubic interpolation).
 *
 *  Extract a cell value from raster map at given northing and easting
 *  with a sampled 3x3 window using a cubic interpolation.
 *
 *  \param fd file descriptor
 *  \param window region settings
 *  \param cats categories
 *  \param north northing position
 *  \param east easting position
 *  \param usedesc flag to scan category label
 *
 *  \return cell value at given position
 */
DCELL Rast_get_sample_cubic(int fd,
			    const struct Cell_head * window,
			    struct Categories * cats,
			    double north, double east, int usedesc)
{
    int i, j, row, col;
    double grid[4][4];
    DCELL *rows[4];
    double frow, fcol, trow, tcol;
    DCELL result;

    for (i = 0; i < 4; i++)
	rows[i] = Rast_allocate_d_buf();

    frow = Rast_northing_to_row(north, window);
    fcol = Rast_easting_to_col(east, window);

    /* convert northing and easting to row and col, resp */
    row = (int)floor(frow - 1.5);
    col = (int)floor(fcol - 1.5);

    trow = frow - row - 1.5;
    tcol = fcol - col - 1.5;

    if (row < 0 || row + 3 >= Rast_window_rows() ||
	col < 0 || col + 3 >= Rast_window_cols()) {
	Rast_set_d_null_value(&result, 1);
	goto done;
    }

    for (i = 0; i < 4; i++)
	Rast_get_d_row(fd, rows[i], row + i);

    for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++)
	    if (Rast_is_d_null_value(&rows[i][col + j])) {
		Rast_set_d_null_value(&result, 1);
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
		G_squeeze(buf =
			  Rast_get_c_cat((CELL *) & (rows[i][col + j]),
					 cats));
		grid[i][j] = scancatlabel(buf);
	    }
	}
    }
    else {
	for (i = 0; i < 4; i++)
	    for (j = 0; j < 4; j++)
		grid[i][j] = rows[i][col + j];
    }

    result = Rast_interp_bicubic(tcol, trow,
				 grid[0][0], grid[0][1], grid[0][2],
				 grid[0][3], grid[1][0], grid[1][1],
				 grid[1][2], grid[1][3], grid[2][0],
				 grid[2][1], grid[2][2], grid[2][3],
				 grid[3][0], grid[3][1], grid[3][2],
				 grid[3][3]);

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

