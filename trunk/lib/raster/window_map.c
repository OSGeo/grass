/*!
 * \file lib/raster/window_map.c
 *
 * \brief Raster Library - Window mapping functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>

#include "R.h"


#define alloc_index(n) (COLUMN_MAPPING *) G_malloc((n)*sizeof(COLUMN_MAPPING))


/*!
 * \brief Create window mapping.
 *
 * Creates mapping from cell header into window. The boundaries and 
 * resolution of the two spaces do not have to be the same or aligned in 
 * any way.
 *
 * \param fd file descriptor
 */
void Rast__create_window_mapping(int fd)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    COLUMN_MAPPING *col;
    int i;
    int x;
    double C1, C2;
    double west, east;

    if (fcb->open_mode >= 0 && fcb->open_mode != OPEN_OLD)	/* open for write? */
	return;
    if (fcb->open_mode == OPEN_OLD)	/* already open ? */
	G_free(fcb->col_map);

    col = fcb->col_map = alloc_index(R__.rd_window.cols);

    /*
     * for each column in the window, go to center of the cell,
     * compute nearest column in the data file
     * if column is not in data file, set column to 0
     *
     * for lat/lon move window so that west is bigger than
     * cellhd west.
     */
    west = R__.rd_window.west;
    east = R__.rd_window.east;
    if (R__.rd_window.proj == PROJECTION_LL) {
	while (west > fcb->cellhd.west + 360.0) {
	    west -= 360.0;
	    east -= 360.0;
	}
	while (west < fcb->cellhd.west) {
	    west += 360.0;
	    east += 360.0;
	}
    }

    C1 = R__.rd_window.ew_res / fcb->cellhd.ew_res;
    C2 = (west - fcb->cellhd.west +
	  R__.rd_window.ew_res / 2.0) / fcb->cellhd.ew_res;
    for (i = 0; i < R__.rd_window.cols; i++) {
	x = C2;
	if (C2 < x)		/* adjust for rounding of negatives */
	    x--;
	if (x < 0 || x >= fcb->cellhd.cols)	/* not in data file */
	    x = -1;
	*col++ = x + 1;
	C2 += C1;
    }

    /* do wrap around for lat/lon */
    if (R__.rd_window.proj == PROJECTION_LL) {
	
	while (east - 360.0 > fcb->cellhd.west) {
	    east -= 360.0;
	    west -= 360.0;

	    col = fcb->col_map;
	    C2 = (west - fcb->cellhd.west +
		  R__.rd_window.ew_res / 2.0) / fcb->cellhd.ew_res;
	    for (i = 0; i < R__.rd_window.cols; i++) {
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
    }

    G_debug(3, "create window mapping (%d columns)", R__.rd_window.cols);
    /*  for (i = 0; i < R__.rd_window.cols; i++)
       fprintf(stderr, "%s%ld", i % 15 ? " " : "\n", (long)fcb->col_map[i]);
       fprintf(stderr, "\n");
     */

    /* compute C1,C2 for row window mapping */
    fcb->C1 = R__.rd_window.ns_res / fcb->cellhd.ns_res;
    fcb->C2 =
	(fcb->cellhd.north - R__.rd_window.north +
	 R__.rd_window.ns_res / 2.0) / fcb->cellhd.ns_res;
}


/*!
 * \brief Loops rows until mismatch?.
 *
 * This routine works fine if the mask is not set. It may give
 * incorrect results with a mask, since the mask row may have a
 * different repeat value. The issue can be fixed by doing it for the
 * mask as well and using the smaller value.
 *
 * \param fd file descriptor
 * \param row starting row
 *
 * \return number of rows completed
 */
int Rast_row_repeat_nomask(int fd, int row)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
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

    while (++row < R__.rd_window.rows) {
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
