#include "globals.h"
#include <grass/raster.h>
#include <grass/display.h>
#include <stdlib.h>

/* initflag: 0 means don't initialize VIEW_MAP1 or VIEW_MAP2 */
int drawcell(View * view, int initflag)
{
    int fd;
    int left, top;
    int ncols, nrows;
    int row;
    DCELL *dcell;
    int read_colors;
    struct Colors *colors;
    char msg[100];

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
	G_free_colors(colors);
	if (G_read_colors(view->cell.name, view->cell.mapset, colors) < 0)
	    return 0;
	/* set_menu_colors(colors); */
    }

    display_title(view);

    set_colors(colors);

    R_standard_color(BLACK);

    if (initflag) {
	Erase_view(VIEW_TITLE1_ZOOM);
	Erase_view(VIEW_TITLE2_ZOOM);
	Erase_view(VIEW_MAP1_ZOOM);
	Erase_view(VIEW_MAP2);
	Erase_view(VIEW_MAP2_ZOOM);
    }

    G_set_window(&view->cell.head);
    nrows = G_window_rows();
    ncols = G_window_cols();

    left = view->cell.left;
    top = view->cell.top;

    R_standard_color(YELLOW);
    Outline_box(top, top + nrows - 1, left, left + ncols - 1);

    if (getenv("NO_DRAW"))
	return 1;

    fd = G_open_cell_old(view->cell.name, view->cell.mapset);
    if (fd < 0)
	return 0;
    dcell = G_allocate_d_raster_buf();

    sprintf(msg, "Displaying %s ...", view->cell.name);
    Menu_msg(msg);

    D_cell_draw_setup(top, top + nrows, left, left + ncols);
    for (row = 0; row < nrows; row++) {
	if (G_get_d_raster_row_nomask(fd, dcell, row) < 0)
	    break;
	D_draw_d_raster(row, dcell, colors);
    }
    D_cell_draw_end();

    /* only set if cell is on the target side (always a group map on the source side) */
    if (view == VIEW_MAP2 || view == VIEW_MAP2_ZOOM)
	cellmap_present = 1;	/* for drawcell */

    G_close_cell(fd);
    G_free(dcell);


    if (colors != &VIEW_MAP1->cell.colors)
	set_colors(&VIEW_MAP1->cell.colors);

    if (initflag) {
	/* initialize for overlay function in drawvect routine */
	D_new_window("warp_map", VIEW_MAP1->top, VIEW_MAP1->bottom,
		     VIEW_MAP1->left, VIEW_MAP1->right);
	return row == nrows;
    }
    return 0;
}

int re_fresh_rast(void)
{
    /* current location side */
    drawcell(VIEW_MAP1, 0);	/* 0 means don't initialize ZOOM panel */

    /* target side */
    Erase_view(VIEW_MAP2);
    Erase_view(VIEW_MAP2_ZOOM);

    select_target_env();
    drawcell(VIEW_MAP2, 0);
    select_current_env();

    return 0;
}
