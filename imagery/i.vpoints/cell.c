#include <unistd.h>
#include "globals.h"

static int use = 1;
static int plot(int, int);
static int cancel(void);
static int choose_cellfile(char *, char *);


int plotcell(int x, int y)
{
    static Objects objects[] = {
	MENU("CANCEL", cancel, &use),
	INFO("Indicate which side should be plotted", &use),
	OTHER(plot, &use),
	{0}
    };
    /*
     * if the target raster map list is ready, ask the user which side
     * should be plotted, otherwise can only plot group files
     */
    if (access(cell_list, 0) == 0)
	Input_pointer(objects);
    else
	plot(VIEW_MAP1->left + 1, 0);

    return 0;
}

static int cancel(void)
{
    return 1;
}


static int plot(int x, int y)
{
    char name[100], mapset[100];
    struct Cell_head cellhd;

    if (x > VIEW_MAP1->left && x < VIEW_MAP1->right) {
	if (!choose_groupfile(name, mapset))
	    return 1;
	if (G_get_cellhd(name, mapset, &cellhd) < 0)
	    return 1;

	Erase_view(VIEW_MAP1_ZOOM);
	VIEW_MAP1_ZOOM->cell.configured = 0;

	G_adjust_window_to_box(&cellhd, &VIEW_MAP1->cell.head,
			       VIEW_MAP1->nrows, VIEW_MAP1->ncols);
	Configure_view(VIEW_MAP1, name, mapset, cellhd.ns_res, cellhd.ew_res);
	drawcell(VIEW_MAP1, 0);
    }
    else if (x > VIEW_MAP2->left && x < VIEW_MAP2->right) {
	if (!choose_cellfile(name, mapset))
	    return 1;
	select_target_env();

	if (G_get_cellhd(name, mapset, &cellhd) < 0) {
	    select_current_env();
	    return 1;
	}

	Erase_view(VIEW_MAP2_ZOOM);
	VIEW_MAP2_ZOOM->cell.configured = 0;

	G_adjust_window_to_box(&cellhd, &VIEW_MAP2->cell.head,
			       VIEW_MAP2->nrows, VIEW_MAP2->ncols);
	Configure_view(VIEW_MAP2, name, mapset, cellhd.ns_res, cellhd.ew_res);
	select_target_env();
	drawcell(VIEW_MAP2, 0);
	select_current_env();
	if (from_screen < 0) {
	    from_flag = 1;
	    from_screen = 0;
	    if (from_keyboard < 0) {
		from_keyboard = 0;
		from_screen = 1;
	    }
	}
    }
    else
	return 0;		/* ignore mouse click */

    display_points(1);
    return 1;
}

static int choose_cellfile(char *name, char *mapset)
{
    return ask_gis_files("raster", cell_list, name, mapset, 1);
}
