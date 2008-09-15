
/**
 * \file window_map.c
 *
 * \brief GIS Library - Window mapping functions.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <stdlib.h>
#include <grass/gis.h>
#include "G.h"


#define alloc_index(n) (COLUMN_MAPPING *) G_malloc((n)*sizeof(COLUMN_MAPPING))


/**
 * \brief Create window mapping.
 *
 * Creates mapping from cell header into window. The boundaries and 
 * resolution of the two spaces do not have to be the same or aligned in 
 * any way.
 *
 * \param[in] fd file descriptor
 * \return always returns 0
 */

int G__create_window_mapping(int fd)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    COLUMN_MAPPING *col;
    int i;
    int x;
    double C1, C2;
    double west;

    G__init_window();

    if (fcb->open_mode >= 0 && fcb->open_mode != OPEN_OLD)	/* open for write? */
	return 0;
    if (fcb->open_mode == OPEN_OLD)	/* already open ? */
	G_free(fcb->col_map);

    col = fcb->col_map = alloc_index(G__.window.cols);

    /*
     * for each column in the window, go to center of the cell,
     * compute nearest column in the data file
     * if column is not in data file, set column to 0
     *
     * for lat/lon move window so that west is bigger than
     * cellhd west.
     */
    west = G__.window.west;
    if (G__.window.proj == PROJECTION_LL) {
	while (west > fcb->cellhd.west + 360.0)
	    west -= 360.0;
	while (west < fcb->cellhd.west)
	    west += 360.0;
    }

    C1 = G__.window.ew_res / fcb->cellhd.ew_res;
    C2 = (west - fcb->cellhd.west +
	  G__.window.ew_res / 2.0) / fcb->cellhd.ew_res;
    for (i = 0; i < G__.window.cols; i++) {
	x = C2;
	if (C2 < x)		/* adjust for rounding of negatives */
	    x--;
	if (x < 0 || x >= fcb->cellhd.cols)	/* not in data file */
	    x = -1;
	*col++ = x + 1;
	C2 += C1;
    }

    /* do wrap around for lat/lon */
    if (G__.window.proj == PROJECTION_LL) {
	col = fcb->col_map;
	C2 = (west - 360.0 - fcb->cellhd.west +
	      G__.window.ew_res / 2.0) / fcb->cellhd.ew_res;
	for (i = 0; i < G__.window.cols; i++) {
	    x = C2;
	    if (C2 < x)		/* adjust for rounding of negatives */
		x--;
	    if (x < 0 || x >= fcb->cellhd.cols)	/* not in data file */
		x = -1;
	    if (*col == 0)	/* only change those not already set */
		*col = x + 1;
	    col++;
	    C2 += C1;
	}
    }

    G_debug(3, "create window mapping (%d columns)", G__.window.cols);
/*  for (i = 0; i < G__.window.cols; i++)
	fprintf(stderr, "%s%ld", i % 15 ? " " : "\n", (long)fcb->col_map[i]);
    fprintf(stderr, "\n");
*/

    /* compute C1,C2 for row window mapping */
    fcb->C1 = G__.window.ns_res / fcb->cellhd.ns_res;
    fcb->C2 =
	(fcb->cellhd.north - G__.window.north +
	 G__.window.ns_res / 2.0) / fcb->cellhd.ns_res;

    return 0;
}


/**
 * \brief Northing to row.
 *
 * Converts a <b>north</b>ing relative to a <b>window</b> to a row.<br>
 * <b>Note:</b> The result is a double. Casting it to an integer will 
 * give the row number.
 *
 * \param[in] north
 * \param[in] window
 * \return row id
 */

double G_northing_to_row(double north, const struct Cell_head *window)
{
    return (window->north - north) / window->ns_res;
}


/**
 * \brief Adjust east longitude.
 *
 * This routine returns an equivalent <b>east</b> that is
 * larger, but no more than 360 larger than the <b>west</b> 
 * coordinate.<br>
 * <b>Note:</b> This routine should be used only with latitude-longitude 
 * coordinates.
 *
 * \param[in,out] east
 * \param[in] west
 * \return east coordinate
 */

double G_adjust_east_longitude(double east, double west)
{
    while (east > west + 360.0)
	east -= 360.0;
    while (east <= west)
	east += 360.0;

    return east;
}


/**
 * \brief Returns east larger than west.
 *
 * If the region projection is <i>PROJECTION_LL</i>, then this routine 
 * returns an equivalent <b>east</b> that is larger, but no more than 
 * 360 degrees larger, than the coordinate for the western edge of the 
 * region. Otherwise no adjustment is made and the original
 * <b>east</b> is returned.
 *
 * \param[in,out] east
 * \param[in] window
 * \return east coordinate
 */

double G_adjust_easting(double east, const struct Cell_head *window)
{
    if (window->proj == PROJECTION_LL) {
	east = G_adjust_east_longitude(east, window->west);
	if (east > window->east && east == window->west + 360)
	    east = window->west;
    }

    return east;
}


/**
 * \brief Easting to column.
 *
 * Converts <b>east</b> relative to a <b>window</b> to a column.<br>
 * <b>Note:</b> The result is a <i>double</i>. Casting it to an 
 * <i>int</i> will give the column number.
 *
 * \param[in] east
 * \param[in] window
 * \return column id
 */

double G_easting_to_col(double east, const struct Cell_head *window)
{
    east = G_adjust_easting(east, window);

    return (east - window->west) / window->ew_res;
}


/**
 * \brief Row to northing.
 *
 * Converts a <b>row</b> relative to a <b>window</b> to a
 * northing.<br>
 * <b>Note:</b> row is a double:
 *  - row+0.0 will return the northing for the northern edge of the row.<br>
 *  - row+0.5 will return the northing for the center of the row.<br>
 *  - row+1.0 will return the northing for the southern edge of the row.<br>
 * <b>Note:</b> The result is a <i>double</i>. Casting it to an 
 * <i>int</i> will give the column number.
 *
 * \param[in] row
 * \param[in] window
 * \return north coordinate
 */

double G_row_to_northing(double row, const struct Cell_head *window)
{
    return window->north - row * window->ns_res;
}


/**
 * \brief Column to easting.
 *
 * Converts a <b>col</b> relative to a <b>window</b> to an easting.<br>
 * <b>Note:</b> <b>col</b> is a <i>double</i>:<br>
 *  - col+0.0 will return the easting for the western edge of the column.<br>
 *  - col+0.5 will return the easting for the center of the column.<br>
 *  - col+1.0 will return the easting for the eastern edge of the column.<br>
 *
 * \param[in] col
 * \param[in] window
 * \return east coordinate
 */

double G_col_to_easting(double col, const struct Cell_head *window)
{
    return window->west + col * window->ew_res;
}


/**
 * \brief Number of rows in active window.
 *
 * This routine returns the number of rows in the active module window. 
 * Before raster files can be read or written, it is necessary to
 * known how many rows are in the active window. For example:<br>
 \code  
  int nrows, cols;
  int row, col; 
  nrows = G_window_rows( ); 
  ncols = G_window_cols( ); 
  for (row = 0; row < nrows; row++)
  {
  <i>read</i> row ...
  for (col = 0; col < ncols; col++)
  {
  process col ...
  }
  }
 \endcode 
 *
 *  \return number of rows
 */

int G_window_rows(void)
{
    G__init_window();

    return G__.window.rows;
}


/**
 * \brief Number of columns in active window.
 *
 * These
 * routines return the number of rows and columns (respectively) in the active
 * module region. Before raster maps can be read or written, it is necessary to
 * known how many rows and columns are in the active region. For example:<br>
 \code  
  int nrows, cols;
  int row, col; 
  nrows = G_window_rows( ); 
  ncols = G_window_cols( ); 
  for (row = 0; row < nrows; row++)
  {
  <i>read</i> row ...
  for (col = 0; col < ncols; col++)
  {
  process col ...
  }
  }
 \endcode 
 *
 * \return number of columns
 */

int G_window_cols(void)
{
    G__init_window();

    return G__.window.cols;
}


/**
 * \brief Initialize window.
 *
 * \return always returns 0
 */

int G__init_window(void)
{
    if (!G__.window_set) {
	G__.window_set = 1;
	G_get_window(&G__.window);
    }

    return 0;
}


/**
 * \brief Loops rows until mismatch?.
 *
 * This routine works fine if the mask is not set. It may give incorrect 
 * results with a mask, since the mask row may have a different repeat 
 * value. The issue can be fixed by doing it for the mask as well and 
 * using the smaller value.
 *
 * \param[in] fd file descriptor
 * \param[in] row starting row
 * \return number of rows completed
 */

int G_row_repeat_nomask(int fd, int row)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    double f;
    int r1, r2;
    int count;

    count = 1;

    /* r1 is the row in the raster map itself.
     * r2 is the next row(s) in the raster map
     * see get_row.c for details on this calculation
     */
    f = row * fcb->C1 + fcb->C2;
    r1 = f;
    if (f < r1)
	r1--;

    while (++row < G__.window.rows) {
	f = row * fcb->C1 + fcb->C2;
	r2 = f;
	if (f < r2)
	    r2--;
	if (r1 != r2)
	    break;

	count++;
    }

    return count;
}
