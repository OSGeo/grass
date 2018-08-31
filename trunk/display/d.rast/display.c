#include <stdlib.h>

#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>

#include "mask.h"
#include "local_proto.h"


static int cell_draw(const char *, struct Colors *, int, int, RASTER_MAP_TYPE);

int display(const char *name,
	    int overlay,
	    char *bg, RASTER_MAP_TYPE data_type, int invert)
{
    struct Colors colors;
    int r, g, b;

    if (Rast_read_colors(name, "", &colors) == -1)
	G_fatal_error(_("Color file for <%s> not available"), name);

    if (bg) {
	if (G_str_to_color(bg, &r, &g, &b) != 1) {
	    G_warning(_("[%s]: No such color"), bg);
	    r = g = b = 255;
	}
	Rast_set_null_value_color(r, g, b, &colors);
    }

    /* Go draw the raster map */
    cell_draw(name, &colors, overlay, invert, data_type);

    /* release the colors now */
    Rast_free_colors(&colors);

    return 0;
}

static int cell_draw(const char *name,
		     struct Colors *colors,
		     int overlay, int invert, RASTER_MAP_TYPE data_type)
{
    int cellfile;
    void *xarray;
    int cur_A_row;
    int ncols, nrows;

    ncols = Rast_window_cols();
    nrows = Rast_window_rows();

    /* Set up the screen, conversions, and graphics */
    D_setup(0);
    D_set_overlay_mode(overlay);

    /* Make sure map is available */
    cellfile = Rast_open_old(name, "");

    /* Allocate space for cell buffer */
    xarray = Rast_allocate_buf(data_type);

    D_raster_draw_begin();

    /* loop for array rows */
    for (cur_A_row = 0; cur_A_row != -1;) {
	G_percent(cur_A_row, nrows, 2);
	/* Get window (array) row currently required */
	Rast_get_row(cellfile, xarray, cur_A_row, data_type);
	mask_raster_array(xarray, ncols, invert, data_type);

	/* Draw the cell row, and get the next row number */
	cur_A_row = D_draw_raster(cur_A_row, xarray, colors, data_type);

    }
    D_raster_draw_end();

    G_percent(nrows, nrows, 2);

    /* Wrap up and return */
    Rast_close(cellfile);
    G_free(xarray);
    return (0);
}

int mask_raster_array(void *xarray,
		      int ncols, int invert, RASTER_MAP_TYPE data_type)
{
    if (data_type == CELL_TYPE)
	mask_cell_array((CELL *) xarray, ncols, &mask, invert);
    else if (data_type == DCELL_TYPE)
	mask_d_cell_array((DCELL *) xarray, ncols, &d_mask, invert);

    return 0;
}
