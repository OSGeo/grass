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
static double raster_sample_nearest (int fd,
                  const struct Cell_head *window, 
                  struct Categories *cats,
                  double north, double east, int usedesc);
static double raster_sample_bilinear (int fd,
                  const struct Cell_head *window, 
                  struct Categories *cats,
                  double north, double east, int usedesc);
static double raster_sample_cubic (int fd,
                  const struct Cell_head *window, 
                  struct Categories *cats,
                  double north, double east, int usedesc);
static double scancatlabel (const char *);
static void raster_row_error(const struct Cell_head *window, double north, double east);


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

double G_get_raster_sample (int fd,
                const struct Cell_head *window,
                struct Categories *cats,
                double north, double east,
                int usedesc, INTERP_TYPE itype)
{
    double retval;

    switch (itype)
    {
    case NEAREST:
        retval = raster_sample_nearest(fd, window, cats, north, east, usedesc);
    break;
    case BILINEAR:
        retval = raster_sample_bilinear(fd, window, cats, north, east, usedesc);
    break;
    case CUBIC:
        retval = raster_sample_cubic(fd, window, cats, north, east, usedesc);
    break;
    default:
	G_fatal_error("G_get_raster_sample: %s",_("Unknown interpolation type"));
    }

    return retval;
}


static double raster_sample_nearest (int fd,
                  const struct Cell_head *window,
                  struct Categories *cats,
                  double north, double east, int usedesc)
{
    int row, col;
    double predicted;
    DCELL *maprow = G_allocate_d_raster_buf();

    /* convert northing and easting to row and col, resp */
    row = (int) G_northing_to_row (north, window);
    col = (int) G_easting_to_col (east, window);

    if (G_get_d_raster_row (fd, maprow, row) < 0)
        raster_row_error(window, north, east);

    if (G_is_d_null_value(&(maprow[col])))
    {
        predicted = 0.0;
    }
    else if (usedesc)
    {
        char *buf = G_get_cat(maprow[col], cats);

        G_squeeze(buf);
        predicted = scancatlabel(buf);
    }
    else
    {
        predicted = maprow[col];
    }

    G_free(maprow);

    return predicted;
}


static double raster_sample_bilinear (int fd,
                  const struct Cell_head *window,
                  struct Categories *cats,
                  double north, double east, int usedesc)
{
    int i, row, col;
    double grid[2][2], tmp1, tmp2;
    DCELL *arow = G_allocate_d_raster_buf ();
    DCELL *brow = G_allocate_d_raster_buf ();

    /* convert northing and easting to row and col, resp */
    row = (int) G_northing_to_row (north, window);
    col = (int) G_easting_to_col (east, window);

    if (G_get_d_raster_row (fd, arow, row) < 0)
        raster_row_error(window, north, east);

    /*-
     * we need 2x2 pixels to do the interpolation. First we decide if we
     * need the previous or next map row
     */
    if (row == 0)
    {
        /* arow is at top, must get row below */
        if (G_get_d_raster_row (fd, brow, row + 1) < 0)
            raster_row_error(window, north, east);
    }
    else if (row+1 == G_window_rows ())
    {
        /* amaprow is at bottom, get the one above it */
        /* brow = arow; */
        for(i = 0; i < G_window_cols(); ++i)
            brow[i] = arow[i];

        row--;

        if (G_get_d_raster_row (fd, arow, row) < 0)
            raster_row_error(window, north, east);
    }
    else if (north - G_row_to_northing ((double) row + 0.5, window) > 0)
    {
        /* north is above a horizontal centerline going through arow */
        /* brow = arow; */
        for(i = 0; i < G_window_cols(); ++i)
            brow[i] = arow[i];

        row--;

        if (G_get_d_raster_row (fd, arow, row) < 0)
            raster_row_error(window, north, east);
    }
    else
    {
        /* north is below a horizontal centerline going through arow */
        if (G_get_d_raster_row (fd, brow, row+1) < 0)
            raster_row_error(window, north, east);
    }

    /*-
     * next, we decide if we need the column to the right or left of the
     * current column using a procedure similar to above
     */
    if (col+1 == G_window_cols())
        col--;
    else if (east - G_col_to_easting((double) col + 0.5, window) < 0)
        col--;

    /*-
     * now were ready to do bilinear interpolation over
     * arow[col], arow[col+1],
     * brow[col], brow[col+1]
     */
 
    if (usedesc)
    {
        char *buf;

        G_squeeze(buf = G_get_cat ((int)arow[col], cats));
        grid[0][0] = scancatlabel (buf);
        G_squeeze(buf = G_get_cat ((int)arow[col+1], cats));
        grid[0][1] = scancatlabel (buf);
        G_squeeze(buf = G_get_cat ((int)brow[col], cats));
        grid[1][0] = scancatlabel (buf);
        G_squeeze(buf = G_get_cat ((int)brow[col+1], cats));
        grid[1][1] = scancatlabel (buf);
    }
    else
    {
        grid[0][0] = (double) arow[col];
        grid[0][1] = (double) arow[col + 1];
        grid[1][0] = (double) brow[col];
        grid[1][1] = (double) brow[col + 1];
    }

    /* Treat NULL's as zero */
    if (G_is_d_null_value(&(arow[col])))
        grid[0][0] = 0.0;
    if (G_is_d_null_value(&(arow[col+1])))
        grid[0][1] = 0.0;
    if (G_is_d_null_value(&(brow[col])))
        grid[1][0] = 0.0;
    if (G_is_d_null_value(&(brow[col+1])))
        grid[1][1] = 0.0;

    east = fabs(G_col_to_easting((double)col, window) - east);
    while (east > window->ew_res)
        east -= window->ew_res;

    north = fabs(G_row_to_northing((double)row, window) - north);
    while (north > window->ns_res)
        north -= window->ns_res;

    /* we do two linear interpolations along the rows */
    tmp1 = G_interp_linear(east / window->ew_res, grid[0][0], grid[0][1]);
    tmp2 = G_interp_linear(east / window->ew_res, grid[1][0], grid[1][1]);

    G_debug(3, "DIAG: r=%d c=%d t1=%g t2=%g\te=%g n=%g",
            row, col, tmp1, tmp2, east, north);
    G_debug(3, "DIAG: %g %g\n      %g %g",
            grid[0][0], grid[0][1], grid[1][0], grid[1][1]);

    /*-
     * Now we interpolate along a line parallel to columns
     * and passing through easting
     */
    G_free(arow);
    G_free(brow);

    return G_interp_linear(north / window->ns_res, tmp1, tmp2);
}


static double raster_sample_cubic (int fd,
                  const struct Cell_head *window, 
                  struct Categories *cats,
                  double north, double east, int usedesc)
{
    int i, row, col;
    double grid[4][4], tmp[4];
    DCELL *arow = G_allocate_d_raster_buf();
    DCELL *brow = G_allocate_d_raster_buf();
    DCELL *crow = G_allocate_d_raster_buf();
    DCELL *drow = G_allocate_d_raster_buf();

    /* convert northing and easting to row and col, resp */
    row = G_northing_to_row(north, window);
    col = G_easting_to_col(east, window);

    if (G_get_d_raster_row(fd, arow, row) < 0)
        raster_row_error(window, north, east);

    /* we need 4x4 pixels to do the interpolation. */
    if (row == 0)
    {
        /* row containing sample is at top, must get three rows below */
        if (G_get_d_raster_row(fd, brow, row + 1) < 0)
            raster_row_error(window, north, east);
        if (G_get_d_raster_row(fd, crow, row + 2) < 0)
            raster_row_error(window, north, east);
        if (G_get_d_raster_row(fd, drow, row + 3) < 0)
            raster_row_error(window, north, east);
    }
    else if (row == 1)
    {
        /* must get row above and tow rows below */
        for (i = 0; i < G_window_cols(); ++i)
            brow[i] = arow[i];

        if (G_get_d_raster_row(fd, arow, row - 1) < 0)
            raster_row_error(window, north, east);
        if (G_get_d_raster_row(fd, crow, row + 1) < 0)
            raster_row_error(window, north, east);
        if (G_get_d_raster_row(fd, drow, row + 2) < 0)
            raster_row_error(window, north, east);

        row--;
    }
    else if (row + 1 == G_window_rows())
    {
        /* arow is at bottom, get the three above it */
        for (i = 0; i < G_window_cols(); ++i)
            drow[i] = arow[i];

        if (G_get_d_raster_row(fd, arow, row - 3) < 0)
            raster_row_error(window, north, east);
        if (G_get_d_raster_row(fd, brow, row - 2) < 0)
            raster_row_error(window, north, east);
        if (G_get_d_raster_row(fd, crow, row - 1) < 0)
            raster_row_error(window, north, east);

        row -= 3;
    }
    else if (row + 2 == G_window_rows())
    {
        /* arow is next to bottom, get the one below and two above it */
        for (i = 0; i < G_window_cols(); ++i)
            crow[i] = arow[i];

        if (G_get_d_raster_row(fd, arow, row - 2) < 0)
            raster_row_error(window, north, east);
        if (G_get_d_raster_row(fd, brow, row - 1) < 0)
            raster_row_error(window, north, east);
        if (G_get_d_raster_row(fd, drow, row + 1) < 0)
            raster_row_error(window, north, east);

        row -= 2;
    }
    else if (north - G_row_to_northing((double)row + 0.5, window) > 0)
    {
        /*
         * north is above a horizontal centerline going through arow. need two
         * above and one below
         */
        for (i = 0; i < G_window_cols(); ++i)
            crow[i] = arow[i];

        if (G_get_d_raster_row(fd, arow, row - 2) < 0)
            raster_row_error(window, north, east);
        if (G_get_d_raster_row(fd, brow, row - 1) < 0)
            raster_row_error(window, north, east);
        if (G_get_d_raster_row(fd, drow, row + 1) < 0)
            raster_row_error(window, north, east);

        row -= 2;
    }
    else
    {
        /*
         * north is below a horizontal centerline going through arow need one
         * above and two below
         */
        for (i = 0; i < G_window_cols(); ++i)
            brow[i] = arow[i];

        if (G_get_d_raster_row(fd, arow, row - 1) < 0)
            raster_row_error(window, north, east);
        if (G_get_d_raster_row(fd, crow, row + 1) < 0)
            raster_row_error(window, north, east);
        if (G_get_d_raster_row(fd, drow, row + 2) < 0)
            raster_row_error(window, north, east);

        row--;
    }

    /*
     * Next, we decide if we need columns to the right and/or left of the
     * current column using a procedure similar to above
     */
    if (col == 0 || col == 1)
        col = 0;
    else if (col + 1 == G_window_cols())
        col -= 3;
    else if (col + 2 == G_window_cols())
        col -= 2;
    else if (east - G_col_to_easting((double)col + 0.5, window) < 0)
        /* east is left of center */
        col -= 2;
    else
        col--;

    /*
     * now were ready to do cubic interpolation over
     * arow[col], arow[col+1], arow[col+2], arow[col+3],
     * brow[col], brow[col+1], brow[col+2], brow[col+3],
     * crow[col], crow[col+1], crow[col+2], crow[col+3],
     * drow[col], drow[col+1], drow[col+2], drow[col+3],
     */

    if (usedesc)
    {
        char *buf;

        for (i = 0; i < 4; ++i) {
            G_squeeze(buf = G_get_cat(arow[col + i], cats));
            grid[0][i] = scancatlabel(buf);
            G_squeeze(buf = G_get_cat(brow[col + i], cats));
            grid[1][i] = scancatlabel(buf);
            G_squeeze(buf = G_get_cat(crow[col + i], cats));
            grid[2][i] = scancatlabel(buf);
            G_squeeze(buf = G_get_cat(drow[col + i], cats));
            grid[3][i] = scancatlabel(buf);
        }
    }
    else
    {
        for (i = 0; i < 4; ++i) {
            grid[0][i] = (double)arow[col + i];
            grid[1][i] = (double)brow[col + i];
            grid[2][i] = (double)crow[col + i];
            grid[3][i] = (double)drow[col + i];
        }
    }

    /* Treat NULL cells as 0.0 */
    for (i = 0; i < 4; i++) {
        if (G_is_d_null_value(&(arow[col + i])))
            grid[0][i] = 0.0;
        if (G_is_d_null_value(&(brow[col + i])))
            grid[1][i] = 0.0;
        if (G_is_d_null_value(&(crow[col + i])))
            grid[2][i] = 0.0;
        if (G_is_d_null_value(&(drow[col + i])))
            grid[3][i] = 0.0;
    }

    /* this needs work here */
    east = fabs(G_col_to_easting((double)col + 1, window) - east);
    while (east > window->ew_res)
        east -= window->ew_res;
    east /= window->ew_res;

    north = fabs(G_row_to_northing((double)row + 1, window) - north);
    while (north > window->ns_res)
        north -= window->ns_res;
    north /= window->ns_res;

    /* we do four cubic convolutions along the rows */
    for (i = 0; i < 4; ++i)
        tmp[i] = G_interp_cubic(east, grid[i][0], grid[i][1], grid[i][2], grid[i][3]);

    G_debug(3, "raster_sample_cubic(): DIAG: (%d,%d) 1=%3.2g 2=%3.2g 3=%3.2g 4=%3.2g\te=%g n=%g",
	    row, col, tmp[0], tmp[1], tmp[2], tmp[3], east, north);

    G_free(arow);
    G_free(brow);
    G_free(crow);
    G_free(drow);

    /* user horner's method again for the final interpolation */
    return G_interp_cubic(north, tmp[0], tmp[1], tmp[2], tmp[3]);
}


static double scancatlabel (const char *str)
{
    double val;

    if (strcmp(str,"no data") != 0)
        sscanf(str, "%lf", &val);
    else
    {
        G_warning(_("\"no data\" label found; setting to zero"));
        val = 0.0;
    }

    return val;
}


static void raster_row_error(const struct Cell_head *window, double north, double east)
{
    G_debug(3, "DIAG: \tRegion is: n=%g s=%g e=%g w=%g",
         window->north, window->south, window->east, window->west);
    G_debug(3, "      \tData point is north=%g east=%g", north, east);

    G_fatal_error(_("Problem reading raster map"));
}
