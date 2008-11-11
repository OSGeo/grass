#include <unistd.h>
#include <grass/glocale.h>
#include "global.h"

/* Modified to support Grass 5.0 fp format 11 april 2000
 *
 * Pierre de Mouveaux - pmx@audiovu.com
 *
 */

int rectify(char *name, char *mapset, char *result, int order)
{
    struct Cell_head cellhd, win;
    int ncols, nrows;
    int row, col;
    int infd;
    void *rast;
    char buf[2048] = "";

    select_current_env();
    if (G_get_cellhd(name, mapset, &cellhd) < 0)
	return 0;

    /* open the result file into target window
     * this open must be first since we change the window later
     * raster maps open for writing are not affected by window changes
     * but those open for reading are
     *
     * also tell open that raster map will have the same format
     * (ie number of bytes per cell) as the file being rectified
     */

    select_target_env();
    G_set_window(&target_window);
    G_set_cell_format(cellhd.format);
    select_current_env();

    /* open the file to be rectified
     * set window to cellhd first to be able to read file exactly
     */
    G_set_window(&cellhd);
    infd = G_open_cell_old(name, mapset);
    if (infd < 0) {
	close(infd);
	return 0;
    }
    map_type = G_get_raster_map_type(infd);
    rast = (void *)G_calloc(G_window_cols() + 1, G_raster_size(map_type));
    G_set_null_value(rast, G_window_cols() + 1, map_type);

    G_copy(&win, &target_window, sizeof(win));

    win.west += win.ew_res / 2;
    ncols = target_window.cols;
    col = 0;

    temp_fd = 0;

    while (ncols > 0) {
	if ((win.cols = ncols) > NCOLS)
	    win.cols = NCOLS;
	win.north = target_window.north - win.ns_res / 2;
	nrows = target_window.rows;
	row = 0;

	while (nrows > 0) {
	    if ((win.rows = nrows) > NROWS)
		win.rows = NROWS;

	    compute_georef_matrix(&cellhd, &win, order);
	    perform_georef(infd, rast);
	    write_matrix(row, col);

	    nrows -= win.rows;
	    row += win.rows;
	    win.north -= (win.ns_res * win.rows);
	}

	ncols -= win.cols;
	col += win.cols;
	win.west += (win.ew_res * win.cols);
	G_percent(col, col + ncols, 1);
    }

    select_target_env();

    if (cellhd.proj == 0) {	/* x,y imagery */
	cellhd.proj = target_window.proj;
	cellhd.zone = target_window.zone;
    }

    if (target_window.proj != cellhd.proj) {
	cellhd.proj = target_window.proj;
	G_warning(_("Raster map <%s@%s>: projection don't match current settings"),
		  name, mapset);
    }

    if (target_window.zone != cellhd.zone) {
	cellhd.zone = target_window.zone;
	G_warning(_("Raster map <%s@%s>: zone don't match current settings"),
		  name, mapset);
    }

    target_window.compressed = cellhd.compressed;
    G_close_cell(infd);		/* (pmx) 17 april 2000 */
    write_map(result);
    select_current_env();

    G_free(rast);

    return 1;
}
