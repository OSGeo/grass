#include <unistd.h>
#include <string.h>

#include <grass/raster.h>
#include <grass/glocale.h>

#include "global.h"

/* Modified to support Grass 5.0 fp format 11 april 2000
 *
 * Pierre de Mouveaux - pmx@audiovu.com
 *
 */

int rectify(char *name, char *mapset, char *result, int order, char *interp_method)
{
    struct Cell_head cellhd;
    int ncols, nrows;
    int row, col;
    double row_idx, col_idx;
    int infd, cell_size, outfd;
    void *trast, *tptr;
    double n1, e1, nx, ex;
    struct cache *ibuffer;

    select_current_env();
    Rast_get_cellhd(name, mapset, &cellhd);

    /* open the file to be rectified
     * set window to cellhd first to be able to read file exactly
     */
    Rast_set_input_window(&cellhd);
    infd = Rast_open_old(name, mapset);
    map_type = Rast_get_map_type(infd);
    cell_size = Rast_cell_size(map_type);

    ibuffer = readcell(infd, seg_mb);

    Rast_close(infd);		/* (pmx) 17 april 2000 */

    G_message(_("Rectify <%s@%s> (location <%s>)"),
	      name, mapset, G_location());
    select_target_env();
    G_message(_("into  <%s@%s> (location <%s>) ..."),
	      result, G_mapset(), G_location());

    nrows = target_window.rows;
    ncols = target_window.cols;

    if (strcmp(interp_method, "nearest") != 0) {
	map_type = DCELL_TYPE;
	cell_size = Rast_cell_size(map_type);
    }

    /* open the result file into target window
     * this open must be first since we change the window later
     * raster maps open for writing are not affected by window changes
     * but those open for reading are
     */

    outfd = Rast_open_new(result, map_type);
    trast = Rast_allocate_output_buf(map_type);

    for (row = 0; row < nrows; row++) {
	n1 = target_window.north - (row + 0.5) * target_window.ns_res;

	G_percent(row, nrows, 2);

	Rast_set_null_value(trast, ncols, map_type);
	tptr = trast;
	for (col = 0; col < ncols; col++) {
	    e1 = target_window.west + (col + 0.5) * target_window.ew_res;

	    /* backwards transformation of target cell center */
	    if (order == 0)
		I_georef_tps(e1, n1, &ex, &nx, E21_t, N21_t, &cp, 0);
	    else
		I_georef(e1, n1, &ex, &nx, E21, N21, order);

	    /* convert to row/column indices of source raster */
	    row_idx = (cellhd.north - nx) / cellhd.ns_res;
	    col_idx = (ex - cellhd.west) / cellhd.ew_res;

	    /* resample data point */
	    interpolate(ibuffer, tptr, map_type, &row_idx, &col_idx, &cellhd);

	    tptr = G_incr_void_ptr(tptr, cell_size);
	}
	Rast_put_row(outfd, trast, map_type);
    }
    G_percent(1, 1, 1);

    Rast_close(outfd);		/* (pmx) 17 april 2000 */
    G_free(trast);

    close(ibuffer->fd);
    G_free(ibuffer);

    Rast_get_cellhd(result, G_mapset(), &cellhd);

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

    select_current_env();

    return 1;
}
