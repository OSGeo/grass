#include "Gwater.h"

int find_pourpts(void)
{
    int row, col;
    double easting, northing, stream_length;
    CELL old_elev, basin_num, value, is_swale;

    basin_num = 0;
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 3);
	northing = window.north - (row + .5) * window.ns_res;
	for (col = 0; col < ncols; col++) {
	    /* cseg_get (&wat, &value, row, col);
	       if (value < 0)
	       {
	       value = -value;
	       } */
	    cseg_get(&asp, &value, row, col);
	    bseg_get(&swale, &is_swale, row, col);
	    if (value < 0 && is_swale > 0) {
		basin_num += 2;
		cseg_get(&alt, &old_elev, row, col);
		if (arm_flag) {
		    easting = window.west + (col + .5) * window.ew_res;
		    fprintf(fp, "%5d drains into %5d at %3d %3d %.3f %.3f",
			    (int)basin_num, 0, row, col, easting, northing);
		}
		if (col == 0 || col == ncols - 1) {
		    stream_length = .5 * window.ew_res;
		}
		else if (row == 0 || row == nrows - 1) {
		    stream_length = .5 * window.ns_res;
		}
		else {
		    stream_length = 0.0;
		}
		basin_num =
		    def_basin(row, col, basin_num, stream_length, old_elev);
	    }
	}
    }
    G_percent(nrows, nrows, 1);	/* finish it */

    return 0;
}
