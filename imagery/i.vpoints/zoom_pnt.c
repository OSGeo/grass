#include <math.h>
#include "globals.h"
#include <grass/display.h>

static View *pick_view, *zoom_view, *main_view;
static int target_flag;
static int zoom1(int, int);
static int cancel(void);

int zoom_point(void)
{
    static int use = 1;

    static Objects objects[] = {
	MENU("CANCEL", cancel, &use),
	INFO(" Mark center of area to be zoomed ", &use),
	OTHER(zoom1, &use),
	{0}
    };

    Input_pointer(objects);
    return 1;
}

static int zoom1(int x, int y)
{				/* called by Input_pointer */
    int top, bottom, left, right;
    int n, row, col;
    int nrows, ncols;
    struct Cell_head cellhd;
    int mag;
    double magnification();
    double north, south, east, west;
    double ns_range, ew_range, pnt_north, pnt_east, x_range, y_range;

    if (In_view(pick_view = VIEW_MAP1, x, y)) {
	main_view = VIEW_MAP1;
	zoom_view = VIEW_MAP1_ZOOM;
	target_flag = 0;
    }
    else if (In_view(pick_view = VIEW_MAP2, x, y)) {
	if (!pick_view->cell.configured)
	    return 0;		/* ignore the mouse event */
	main_view = VIEW_MAP2;
	zoom_view = VIEW_MAP2_ZOOM;
	target_flag = 1;
    }
    else if (In_view(pick_view = VIEW_MAP1_ZOOM, x, y)) {
	if (!pick_view->cell.configured)
	    return 0;		/* ignore the mouse event */
	main_view = VIEW_MAP1;
	zoom_view = VIEW_MAP1_ZOOM;
	target_flag = 0;
    }
    else if (In_view(pick_view = VIEW_MAP2_ZOOM, x, y)) {
	if (!pick_view->cell.configured)
	    return 0;		/* ignore the mouse event */
	main_view = VIEW_MAP2;
	zoom_view = VIEW_MAP2_ZOOM;
	target_flag = 1;
    }
    else
	return 0;		/* ignore the mouse event */
    if (!pick_view->cell.configured)
	return 0;		/* just to be sure */
    /*
     * make sure point is within edges of image as well
     */
    if (x <= pick_view->cell.left)
	return 0;
    if (x >= pick_view->cell.right)
	return 0;
    if (y <= pick_view->cell.top)
	return 0;
    if (y >= pick_view->cell.bottom)
	return 0;

    /*
     *
     ok, erase menu messages
     */
    Menu_msg("");

    /* determine magnification of zoom */
    if (zoom_view->cell.configured) {
	if (zoom_view == pick_view)
	    mag = floor(magnification(zoom_view) + 1.0) + .1;
	else
	    mag = ceil(magnification(zoom_view)) + .1;
    }
    else {
	mag = floor(magnification(main_view) + 1.0) + .1;
    }
    if (!ask_magnification(&mag))
	return 1;
    /* 
     * Determine the zoom window (ie, cellhd)
     */

    G_copy(&cellhd, &main_view->cell.head, sizeof(cellhd));

    if (target_flag == 0) {
	cellhd.ns_res = main_view->cell.ns_res / mag;
	cellhd.ew_res = main_view->cell.ew_res / mag;
    }

    cellhd.cols = (cellhd.east - cellhd.west) / cellhd.ew_res;
    cellhd.rows = (cellhd.north - cellhd.south) / cellhd.ns_res;

    /* convert x,y to col,row */

    col = view_to_col(pick_view, x);
    east = col_to_easting(&pick_view->cell.head, col, 0.5);
    col = easting_to_col(&cellhd, east);

    row = view_to_row(pick_view, y);
    north = row_to_northing(&pick_view->cell.head, row, 0.5);
    row = northing_to_row(&cellhd, north);

    if (target_flag == 0) {
	ncols = zoom_view->ncols;
	nrows = zoom_view->nrows;

	n = cellhd.cols - col;
	if (n > col)
	    n = col;
	if (n + n + 1 >= ncols) {
	    n = ncols / 2;
	    if (n + n + 1 >= ncols)
		n--;
	}
	left = col - n;
	right = col + n;

	n = cellhd.rows - row;
	if (n > row)
	    n = row;
	if (n + n + 1 >= nrows) {
	    n = nrows / 2;
	    if (n + n + 1 >= nrows)
		n--;
	}

	top = row - n;
	bottom = row + n;

	n = cellhd.rows - row;
	north = row_to_northing(&cellhd, top, 0.0);
	west = col_to_easting(&cellhd, left, 0.0);
	south = row_to_northing(&cellhd, bottom, 1.0);
	east = col_to_easting(&cellhd, right, 1.0);

	cellhd.north = north;
	cellhd.south = south;
	cellhd.east = east;
	cellhd.west = west;

	cellhd.rows = (cellhd.north - cellhd.south) / cellhd.ns_res;
	cellhd.cols = (cellhd.east - cellhd.west) / cellhd.ew_res;

	/*
	 * Outline the zoom window on the main map
	 * Turn previous one to grey.
	 */
	if (zoom_view->cell.configured) {
	    R_standard_color(GREY);
	    Outline_cellhd(main_view, &zoom_view->cell.head);
	}

	R_standard_color(YELLOW);
	Outline_cellhd(main_view, &cellhd);

    }
    else {
	top = pick_view->cell.top;
	bottom = pick_view->cell.bottom;
	left = pick_view->cell.left;
	right = pick_view->cell.right;

	north = cellhd.north;
	south = cellhd.south;
	east = cellhd.east;
	west = cellhd.west;

	ns_range = north - south;
	ew_range = east - west;
	y_range = (double)(bottom - top);
	x_range = (double)(right - left);

	pnt_north = north - (ns_range / y_range) * (double)row;
	pnt_east = east - (ew_range / x_range) * (double)(x_range - col);

	north = pnt_north + ns_range / mag / 2.0;
	south = pnt_north - ns_range / mag / 2.0;
	east = pnt_east + ew_range / mag / 2.0;
	west = pnt_east - ew_range / mag / 2.0;

	cellhd.north = north;
	cellhd.south = south;
	cellhd.east = east;
	cellhd.west = west;

	cellhd.rows = zoom_view->bottom - zoom_view->top + 1;
	cellhd.cols = zoom_view->right - zoom_view->left + 1;
	cellhd.ns_res = (north - south) / cellhd.rows;
	cellhd.ew_res = (east - west) / cellhd.cols;

	if (cellhd.ns_res > cellhd.ew_res)
	    cellhd.ew_res = cellhd.ns_res;
	else
	    cellhd.ns_res = cellhd.ew_res;

	/*
	 * Outline the zoom window on the main map
	 * Turn previous one to grey.
	 */
	if (zoom_view->cell.configured) {
	    R_standard_color(GREY);
	    Outline_cellhd(main_view, &zoom_view->cell.head);
	}

    }

    /*
     * zoom
     */
    if (target_flag)
	select_target_env();

    G_copy(&zoom_view->cell.head, &cellhd, sizeof(cellhd));

    if (target_flag) {
	R_standard_color(YELLOW);
	Outline_cellhd(VIEW_MAP2, &zoom_view->cell.head);

	zoom_view->cell.ns_res = cellhd.ns_res;
	zoom_view->cell.ew_res = cellhd.ew_res;

	G_adjust_window_to_box(&cellhd, &zoom_view->cell.head,
			       zoom_view->nrows, zoom_view->ncols);
    }

    Configure_view(zoom_view, pick_view->cell.name, pick_view->cell.mapset,
		   pick_view->cell.ns_res, pick_view->cell.ew_res);

    if (target_flag) {
	zoomvect(zoom_view);
	VIEW_MAP2_ZOOM->cell.configured = 1;
    }
    else
	drawcell(zoom_view, 0);

    select_current_env();
    display_points(1);

    return 1;			/* pop back */
}


static int cancel(void)
{
    return -1;
}
