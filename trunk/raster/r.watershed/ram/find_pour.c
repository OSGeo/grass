#include "Gwater.h"

int find_pourpts(void)
{
    int value, row, col;
    double easting, northing, stream_length;
    CELL old_elev, basin_num;

    ocs_alloced = 2 * bas_thres;
    ocs = (OC_STACK *) G_malloc(ocs_alloced * sizeof(OC_STACK));

    basin_num = 0;
    stream_length = old_elev = 0;
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 1);
	northing = window.north - (row + .5) * window.ns_res;
	for (col = 0; col < ncols; col++) {
	    value = FLAG_GET(swale, row, col);
	    if (value && asp[SEG_INDEX(asp_seg, row, col)] <= 0) {
		basin_num += 2;
		if (arm_flag) {
		    easting = window.west + (col + .5) * window.ew_res;
		    fprintf(fp, "%5d drains into %5d at %3d %3d %.3f %.3f",
			    (int)basin_num, 0, row, col, easting, northing);
		    if (col == 0 || col == ncols - 1) {
			stream_length = .5 * window.ew_res;
		    }
		    else if (row == 0 || row == nrows - 1) {
			stream_length = .5 * window.ns_res;
		    }
		    else {
			stream_length = 0.0;
		    }
		    old_elev = alt[SEG_INDEX(alt_seg, row, col)];
		}
		basin_num =
		    def_basin(row, col, basin_num, stream_length, old_elev);
	    }
	}
    }
    G_percent(nrows, nrows, 1);	/* finish it */
    n_basins = basin_num;
    G_free(ocs);

    return 0;
}
