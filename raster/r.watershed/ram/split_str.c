#include "Gwater.h"

CELL
split_stream(int row, int col, int new_r[], int new_c[], int ct,
	     CELL basin_num, double stream_length, CELL old_elev)
{
    CELL downdir, old_basin, new_elev, aspect;
    double slope, easting, northing;
    int doit, ctr, updir, splitdir[9];
    int thisdir, leftflag, riteflag;
    int r, c, rr, cc;

    new_elev = 0;

    for (ctr = 1; ctr <= ct; ctr++)
	splitdir[ctr] = drain[row - new_r[ctr] + 1][col - new_c[ctr] + 1];
    updir = splitdir[1];
    downdir = asp[SEG_INDEX(asp_seg, row, col)];
    if (downdir < 0)
	downdir = -downdir;
    riteflag = leftflag = 0;
    for (r = row - 1, rr = 0; rr < 3; r++, rr++) {
	for (c = col - 1, cc = 0; cc < 3; c++, cc++) {
	    if (r >= 0 && c >= 0 && r < nrows && c < ncols) {
		if (r == row && c == col)
		    continue;
		aspect = asp[SEG_INDEX(asp_seg, r, c)];
		if (aspect == drain[rr][cc]) {
		    doit = 1;
		    thisdir = updrain[rr][cc];
		    for (ctr = 1; ctr <= ct; ctr++) {
			if (thisdir == splitdir[ctr]) {
			    doit = 0;
			    ctr = ct;
			}
		    }
		    if (doit) {
			thisdir = updrain[rr][cc];
			switch (haf_basin_side(updir, (int)downdir, thisdir)) {
			case LEFT:
			    overland_cells(r, c, basin_num, basin_num - 1,
					   &new_elev);
			    leftflag++;
			    break;
			case RITE:
			    overland_cells(r, c, basin_num, basin_num,
					   &new_elev);
			    riteflag++;
			    break;
			}
		    }
		}
	    }
	}
    }
    if (leftflag >= riteflag) {
	haf[SEG_INDEX(haf_seg, row, col)] = basin_num - 1;
    }
    else {
	haf[SEG_INDEX(haf_seg, row, col)] = basin_num;
    }
    old_basin = basin_num;
    if (arm_flag) {
	new_elev = alt[SEG_INDEX(alt_seg, row, col)];
	if ((slope = (new_elev - old_elev) / stream_length) < MIN_SLOPE)
	    slope = MIN_SLOPE;
	fprintf(fp, " %f %f\n", slope, stream_length);
    }
    for (r = 1; r <= ct; r++) {
	basin_num += 2;
	if (arm_flag) {
	    easting = window.west + (new_c[r] + .5) * window.ew_res;
	    northing = window.north - (new_r[r] + .5) * window.ns_res;
	    fprintf(fp, "%5d drains into %5d at %3d %3d %.3f %.3f",
		    (int)basin_num, old_basin, new_r[r], new_c[r], easting,
		    northing);
	}
	if (new_r[r] != row && new_c[r] != col)
	    basin_num =
		def_basin(new_r[r], new_c[r], basin_num, diag, new_elev);
	else if (new_r[r] != row)
	    basin_num =
		def_basin(new_r[r], new_c[r], basin_num, window.ns_res,
			  new_elev);
	else
	    basin_num =
		def_basin(new_r[r], new_c[r], basin_num, window.ew_res,
			  new_elev);
    }
    return (basin_num);
}
