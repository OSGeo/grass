#include "globals.h"
#include <grass/raster.h>

static int x1, y1, x2, y2;
static View *pick_view, *zoom_view, *main_view;
static int target_flag;

static int zoom2(int, int);
static int zoom1(int, int);

static int cancel(void)
{
    return -1;
}

int zoom_box(void)
{
    static int use = 1;


    static Objects objects[] = {
	MENU("CANCEL", cancel, &use),
	INFO(" Mark first corner of region ", &use),
	OTHER(zoom1, &use),
	{0}
    };

    Input_pointer(objects);
    return 1;
}

static int zoom1(int x, int y)
{				/* called by Input_pointer */
    static int use = 1;
    static Objects objects[] = {
	MENU("CANCEL", cancel, &use),
	INFO(" Define the region ", &use),
	OTHER(zoom2, &use),
	{0}
    };

    /* 
     * user has marked first corner 
     * this determines which view is being zoomed
     */
    x1 = x;
    y1 = y;

    if (In_view(pick_view = VIEW_MAP1, x1, y1)) {
	main_view = VIEW_MAP1;
	zoom_view = VIEW_MAP1_ZOOM;
	target_flag = 0;
    }
    else if (In_view(pick_view = VIEW_MAP2, x1, y1)) {
	if (!pick_view->cell.configured)
	    return 0;		/* ignore the mouse event */
	main_view = VIEW_MAP2;
	zoom_view = VIEW_MAP2_ZOOM;
	target_flag = 1;
    }
    else if (In_view(pick_view = VIEW_MAP1_ZOOM, x1, y1)) {
	if (!pick_view->cell.configured)
	    return 0;		/* ignore the mouse event */
	main_view = VIEW_MAP1;
	zoom_view = VIEW_MAP1_ZOOM;
	target_flag = 0;
    }
    else if (In_view(pick_view = VIEW_MAP2_ZOOM, x1, y1)) {
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

    return Input_box(objects, x, y);
}

static int zoom2(int x, int y)
{
    int top, bottom, left, right;
    int row, col;
    struct Cell_head cellhd;

    x2 = x;
    y2 = y;
    /* 
     * user has completed the zoom window.
     * must be in same view as first corner
     */
    if (x1 == x2 || y1 == y2)
	return 0;		/* ignore event */
    if (!In_view(pick_view, x2, y2))
	return 0;
    /*
     * ok, erase menu messages
     */
    Menu_msg("");

    /*
     * assign window coordinates to top,bottom,left,right
     */
    if (x1 < x2) {
	left = x1;
	right = x2;
    }
    else {
	left = x2;
	right = x1;
    }
    if (y1 < y2) {
	top = y1;
	bottom = y2;
    }
    else {
	top = y2;
	bottom = y1;
    }

    /* 
     * Determine the zoom window (ie, cellhd)
     * must copy the current view cellhd first, to preserve header info
     * (such as projection, zone, and other items.)
     * compute zoom window northings,eastings, rows, cols, and resolution
     */

    G_copy(&cellhd, &pick_view->cell.head, sizeof(cellhd));

    /* convert top to northing at top edge of cell
     * left to easting at left edge
     */
    col = view_to_col(pick_view, left);
    row = view_to_row(pick_view, top);
    cellhd.north = row_to_northing(&pick_view->cell.head, row, 0.0);
    cellhd.west = col_to_easting(&pick_view->cell.head, col, 0.0);

    /* convert bottom to northing at bottom edge of cell
     * right to easting at right edge
     */

    col = view_to_col(pick_view, right);
    row = view_to_row(pick_view, bottom);
    cellhd.south = row_to_northing(&pick_view->cell.head, row, 1.0);
    cellhd.east = col_to_easting(&pick_view->cell.head, col, 1.0);

    cellhd.rows = bottom - top + 1;
    cellhd.cols = right - left + 1;
    cellhd.ns_res = (cellhd.north - cellhd.south) / cellhd.rows;
    cellhd.ew_res = (cellhd.east - cellhd.west) / cellhd.cols;

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

    /*
     * zoom
     */
    if (target_flag) {
	select_target_env();
	G_copy(&zoom_view->cell.head, &cellhd, sizeof(cellhd));
	zoom_view->cell.ns_res = cellhd.ns_res;
	zoom_view->cell.ew_res = cellhd.ew_res;
    }


    G_adjust_window_to_box(&cellhd, &zoom_view->cell.head, zoom_view->nrows,
			   zoom_view->ncols);

    Configure_view(zoom_view, pick_view->cell.name, pick_view->cell.mapset,
		   pick_view->cell.ns_res, pick_view->cell.ew_res);

    if (target_flag) {
	if (cellmap_present)
	    drawcell(zoom_view, 0);
	zoomvect(zoom_view);
	VIEW_MAP2_ZOOM->cell.configured = 1;
    }
    else
	drawcell(zoom_view, 0);

    select_current_env();
    display_points(1);

    return 1;			/* pop back */
}
