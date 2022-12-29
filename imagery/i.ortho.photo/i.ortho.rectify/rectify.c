/* rectification code */

/* 1/2002: updated to GRASS 5 write routines and 
   CELL/FP elevation - Markus Neteler
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "global.h"

int rectify(struct Ortho_Image_Group *group, char *name, char *mapset,
            struct cache *ebuffer, double aver_z, char *result,
	    char *interp_method)
{
    struct Cell_head cellhd;
    int ncols, nrows;
    int row, col;
    double row_idx, col_idx;
    int infd, outfd;
    RASTER_MAP_TYPE map_type;
    int cell_size;
    void *trast, *tptr;
    double n1, e1, z1;
    double nx, ex, nx1, ex1, zx1;
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

    ibuffer = readcell(infd, seg_mb_img, 0);

    Rast_close(infd);		/* (pmx) 17 april 2000 */

    G_message(_("Rectify <%s@%s> (location <%s>)"),
	      name, mapset, G_location());
    select_target_env();
    G_set_window(&target_window);
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
	    DCELL *zp = CPTR(ebuffer, row, col);

	    e1 = target_window.west + (col + 0.5) * target_window.ew_res;
	    
	    /* if target cell has no elevation, set to aver_z */
	    if (Rast_is_d_null_value(zp)) {
		G_warning(_("No elevation available at row = %d, col = %d"), row, col);
		z1 = aver_z;
	    }
	    else
		z1 = *zp;

	    /* target coordinates e1, n1 to photo coordinates ex1, nx1 */
	    I_ortho_ref(e1, n1, z1, &ex1, &nx1, &zx1, &group->camera_ref,
			group->XC, group->YC, group->ZC, group->M);

	    G_debug(5, "\t\tAfter ortho ref (photo cords): ex = %f \t nx =  %f",
		    ex1, nx1);

	    /* photo coordinates ex1, nx1 to image coordinates ex, nx */
	    I_georef(ex1, nx1, &ex, &nx, group->E21, group->N21, 1);

	    G_debug(5, "\t\tAfter geo ref: ex = %f \t nx =  %f", ex, nx);

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
    release_cache(ibuffer);

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
