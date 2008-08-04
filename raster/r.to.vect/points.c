/* Written by Bill Brown, USA-CERL, NCSA, UI GMSL.
 */

#include <stdlib.h>
#include <strings.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include "global.h"


int extract_points(int z_flag)
{
    struct line_pnts *points = Vect_new_line_struct();
    CELL *cellbuf;
    FCELL *fcellbuf;
    DCELL *dcellbuf;
    int row, col;
    double x, y;
    int count;

    switch (data_type) {
    case CELL_TYPE:
	cellbuf = G_allocate_c_raster_buf();
	break;
    case FCELL_TYPE:
	fcellbuf = G_allocate_f_raster_buf();
	break;
    case DCELL_TYPE:
	dcellbuf = G_allocate_d_raster_buf();
	break;
    }

    G_message(_("Extracting points..."));

    count = 1;
    for (row = 0; row < cell_head.rows; row++) {
	G_percent(row, n_rows, 2);

	y = G_row_to_northing((double)(row + .5), &cell_head);

	switch (data_type) {
	case CELL_TYPE:
	    G_get_c_raster_row(input_fd, cellbuf, row);
	    break;
	case FCELL_TYPE:
	    G_get_f_raster_row(input_fd, fcellbuf, row);
	    break;
	case DCELL_TYPE:
	    G_get_d_raster_row(input_fd, dcellbuf, row);
	    break;
	}

	for (col = 0; col < cell_head.cols; col++) {
	    int cat, val;
	    double dval;

	    x = G_col_to_easting((double)(col + .5), &cell_head);

	    switch (data_type) {
	    case CELL_TYPE:
		if (G_is_c_null_value(cellbuf + col))
		    continue;
		val = cellbuf[col];
		dval = val;
		break;
	    case FCELL_TYPE:
		if (G_is_f_null_value(fcellbuf + col))
		    continue;
		dval = fcellbuf[col];
		break;
	    case DCELL_TYPE:
		if (G_is_d_null_value(dcellbuf + col))
		    continue;
		dval = dcellbuf[col];
		break;
	    }

	    /* value_flag is used only for CELL type */
	    cat = (value_flag) ? val : count;

	    Vect_reset_line(points);
	    Vect_reset_cats(Cats);
	    Vect_cat_set(Cats, 1, cat);

	    Vect_append_point(points, x, y, dval);
	    Vect_write_line(&Map, GV_POINT, points, Cats);

	    if ((driver != NULL) && !value_flag) {
		insert_value(cat, val, dval);
	    }

	    count++;
	}
    }

    G_percent(row, n_rows, 2);

    return (1);
}
