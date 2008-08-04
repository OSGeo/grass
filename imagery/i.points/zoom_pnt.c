#include "globals.h"
#include "local_proto.h"
#include <grass/raster.h>

static int zoom1(int, int);
static int cancel(void);

static View *pick_view, *zoom_view, *main_view;
static int target_flag;
double ceil();
double floor();

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
     * ok, erase menu messages
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
     * Determine the the zoom window (ie, cellhd)
     */

    G_copy(&cellhd, &main_view->cell.head, sizeof(cellhd));
    cellhd.ns_res = main_view->cell.ns_res / mag;
    cellhd.ew_res = main_view->cell.ew_res / mag;
    cellhd.cols = (cellhd.east - cellhd.west) / cellhd.ew_res;
    cellhd.rows = (cellhd.north - cellhd.south) / cellhd.ns_res;


    /* convert x,y to col,row */

    col = view_to_col(pick_view, x);
    east = col_to_easting(&pick_view->cell.head, col, 0.5);
    col = easting_to_col(&cellhd, east);

    row = view_to_row(pick_view, y);
    north = row_to_northing(&pick_view->cell.head, row, 0.5);
    row = northing_to_row(&cellhd, north);

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
    R_standard_color(RED);
    Outline_cellhd(main_view, &cellhd);


    /*
     * zoom
     */
    if (target_flag)
	select_target_env();
    G_copy(&zoom_view->cell.head, &cellhd, sizeof(cellhd));
    Configure_view(zoom_view, pick_view->cell.name, pick_view->cell.mapset,
		   pick_view->cell.ns_res, pick_view->cell.ew_res);
    drawcell(zoom_view);
    select_current_env();
    display_points(1);

    return 1;			/* pop back */
}


static int cancel(void)
{
    return -1;
}
