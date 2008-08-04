/*

   Chris Rewerts, Agricultural Engineering, Purdue University
   April 1991

   First step to edit is to create a file with the 
   cell layer's data that we can read and write to
   randomly. We also get the original cell layer's 
   other pertinent data (colr, cats) into memory
   since we'd lose it otherwise

 */

#include "edit.h"

int make_temporary_file(void)
{

    void *rast;
    int cellfd;
    int tmpfd;
    int row;
    char line[200];

    G_set_window(&real_window);

    /* read cat and colr support files (if any) info into memory */

    G_suppress_warnings(1);
    colr_ok = G_read_colors(current_name, current_mapset, &colr) > 0;
    cats_ok = G_read_cats(current_name, current_mapset, &cats) >= 0;
    if ((map_type =
	 G_raster_map_type(current_name, current_mapset)) != CELL_TYPE)
	quant_ok = G_read_quant(current_name, current_mapset, &quant);
    else
	quant_ok = 0;
    G_suppress_warnings(0);

    rast = G_allocate_raster_buf(map_type);

    /* open the original file */
    cellfd = G_open_cell_old(current_name, current_mapset);
    if (cellfd < 0) {
	sprintf(line, "unable to open [%s] in [%s]\n", current_name,
		current_mapset);
	error(1, line);
    }
    /* generate a temporary file name and create it */
    tempfile = G_tempfile();
    tmpfd = creat(tempfile, 0666);
    /* how many bytes per cell value */
    cellsize = G_raster_size(map_type);

    /* locate read/write pointer at beginning of temporary file */

    lseek(tmpfd, 0L, 0);

    fprintf(stderr, "\n     +-------------------------------------------+\n");
    fprintf(stderr, "     |         Creating temporary edit file      |\n");
    fprintf(stderr, "     +---------------------------------------");

    for (row = 0; row < real_nrows; row++) {
	G_get_raster_row_nomask(cellfd, rast, row, map_type);
	if (write(tmpfd, rast, real_ncols * cellsize) !=
	    (real_ncols * cellsize))
	    error(1, "error writing temporary file during copy");
	G_percent(row, real_nrows, 5);
    }
    G_percent(100, 100, 5);
    fprintf(stderr, "\n");
    G_close_cell(cellfd);

    close(tmpfd);

    return 0;
}
