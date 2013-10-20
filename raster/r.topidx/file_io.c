#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "global.h"

void read_cells(void)
{
    int fd, i, j;
    RASTER_MAP_TYPE data_type;
    CELL *ccell = NULL;
    FCELL *fcell = NULL;
    struct Cell_head inhead;
    char buf_wrns[32], buf_wrew[32], buf_mrns[32], buf_mrew[32];

    fd = Rast_open_old(input, "");

    data_type = Rast_get_map_type(fd);
    Rast_get_cellhd(input, "", &inhead);

    if (data_type == CELL_TYPE)
	ccell = (CELL *) G_malloc(sizeof(CELL) * window.cols);
    else if (data_type == FCELL_TYPE)
	fcell = (FCELL *) G_malloc(sizeof(FCELL) * window.cols);

    cell = (DCELL **) G_malloc(sizeof(DCELL *) * window.rows);
    atb = (DCELL **) G_malloc(sizeof(DCELL *) * window.rows);
    a = (DCELL **) G_malloc(sizeof(DCELL *) * window.rows);

    if (window.ew_res < inhead.ew_res || window.ns_res < inhead.ns_res) {
	G_format_resolution(window.ew_res, buf_wrew, G_projection());
	G_format_resolution(window.ns_res, buf_wrns, G_projection());
	G_format_resolution(inhead.ew_res, buf_mrew, G_projection());
	G_format_resolution(inhead.ns_res, buf_mrns, G_projection());
	G_fatal_error(_("The current region resolution [%s x %s] is finer "
			"than the input map's resolution [%s x %s]. "
			"The current region resolution must be identical "
			"to, or coarser than, the input map's resolution."),
		      buf_wrew, buf_wrns, buf_mrew, buf_mrns);
    }

    G_message(_("Reading elevation map..."));

    for (i = 0; i < window.rows; i++) {
	G_percent(i, window.rows, 2);

	cell[i] = (DCELL *) G_malloc(sizeof(DCELL) * window.cols);
	atb[i] = (DCELL *) G_malloc(sizeof(DCELL) * window.cols);
	a[i] = (DCELL *) G_malloc(sizeof(DCELL) * window.cols);

	if (data_type == CELL_TYPE) {
	    Rast_get_c_row(fd, ccell, i);
	    for (j = 0; j < window.cols; j++) {
		if (Rast_is_c_null_value(&ccell[j]))
		    Rast_set_d_null_value(&cell[i][j], 1);
		else
		    cell[i][j] = (DCELL) ccell[j];
	    }
	}
	else if (data_type == FCELL_TYPE) {
	    Rast_get_f_row(fd, fcell, i);
	    for (j = 0; j < window.cols; j++) {
		if (Rast_is_f_null_value(&fcell[j]))
		    Rast_set_d_null_value(&cell[i][j], 1);
		else
		    cell[i][j] = (DCELL) fcell[j];
	    }
	}
	else
	    Rast_get_d_row(fd, cell[i], i);
    }
    if (data_type == CELL_TYPE)
	G_free(ccell);
    else if (data_type == FCELL_TYPE)
	G_free(fcell);
    G_percent(i, window.rows, 2);
    Rast_close(fd);
}

void write_cells(void)
{
    int fd, i;
    struct History history;

    fd = Rast_open_new(output, DCELL_TYPE);

    G_message(_("Writing topographic index map..."));

    for (i = 0; i < window.rows; i++) {
	G_percent(i, window.rows, 2);
	Rast_put_d_row(fd, atb[i]);
    }
    G_percent(i, window.rows, 2);
    Rast_close(fd);

    Rast_short_history(output, "raster", &history);
    Rast_set_history(&history, HIST_DATSRC_1, input);
    Rast_write_history(output, &history);
}
