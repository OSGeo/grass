#include <stdlib.h>
#include <grass/gis.h>
#include "globals.h"
#include <grass/display.h>
#include "local_proto.h"

int drawcell(View * view)
{
    int fd;
    int left, top;
    int ncols, nrows;
    int row;
    DCELL *dcell;
    struct Colors *colors;
    int read_colors;
    char msg[GNAME_MAX];


    if (!view->cell.configured)
	return 0;
    if (view == VIEW_MAP1 || view == VIEW_MAP1_ZOOM) {
	colors = &VIEW_MAP1->cell.colors;
	read_colors = view == VIEW_MAP1;
    }
    else {
	colors = &VIEW_MAP2->cell.colors;
	read_colors = view == VIEW_MAP2;
    }
    if (read_colors) {
	Rast_free_colors(colors);
	if (Rast_read_colors(view->cell.name, view->cell.mapset, colors) < 0)
	    return 0;
    }

    display_title(view);

    set_colors(colors);

    Rast_set_window(&view->cell.head);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    left = view->cell.left;
    top = view->cell.top;

    R_standard_color(BLUE);
    Outline_box(top, top + nrows - 1, left, left + ncols - 1);

    if (getenv("NO_DRAW"))
	return 1;

    fd = Rast_open_old(view->cell.name, view->cell.mapset);
    dcell = Rast_allocate_d_buf();

    sprintf(msg, "Plotting %s ...", view->cell.name);
    Menu_msg(msg);

    D_cell_draw_setup(top, top + nrows, left, left + ncols);
    for (row = 0; row < nrows; row++) {
	Rast_get_d_row_nomask(fd, dcell, row);
	D_draw_d_raster(row, dcell, colors);
    }
    D_cell_draw_end();
    Rast_close(fd);
    G_free(dcell);
    if (colors != &VIEW_MAP1->cell.colors)
	set_colors(&VIEW_MAP1->cell.colors);

    return row == nrows;
}
