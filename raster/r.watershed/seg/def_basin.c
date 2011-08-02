#include "Gwater.h"

CELL
def_basin(int row, int col, CELL basin_num, double stream_length,
	  CELL old_elev)
{
    int r, rr, c, cc, ct, new_r[9], new_c[9];
    CELL value, new_elev;
    char downdir, direction, asp_value, cvalue;
    int oldupdir, riteflag, leftflag, thisdir;

    for (;;) {
	cseg_put(&bas, &basin_num, row, col);
	ct = 0;
	for (r = row - 1, rr = 0; rr < 3; r++, rr++) {
	    for (c = col - 1, cc = 0; cc < 3; c++, cc++) {
		if (r >= 0 && c >= 0 && r < nrows && c < ncols) {
		    if (r == row && c == col)
			continue;
		    bseg_get(&asp, &asp_value, r, c);
		    if (asp_value < -1)
			asp_value = -asp_value;
		    if (asp_value == drain[rr][cc]) {
			bseg_get(&bitflags, &cvalue, r, c);
			if (FLAG_GET(cvalue, SWALEFLAG)) {
			    new_r[++ct] = r;
			    new_c[ct] = c;
			}
		    }
		}
	    }
	}
	if (ct == 0) {
	    no_stream(row, col, basin_num, stream_length, old_elev);
	    return (basin_num);
	}
	if (ct >= 2) {
	    basin_num = split_stream(row, col, new_r, new_c, ct,
				     basin_num, stream_length, old_elev);
	    return (basin_num);
	}
	oldupdir = drain[row - new_r[1] + 1][col - new_c[1] + 1];
	bseg_get(&asp, &downdir, row, col);
	if (downdir < 0)
	    downdir = -downdir;
	riteflag = leftflag = 0;
	for (r = row - 1, rr = 0; rr < 3; r++, rr++) {
	    for (c = col - 1, cc = 0; cc < 3; c++, cc++) {
		if (r >= 0 && c >= 0 && r < nrows && c < ncols) {
		    if (r == row && c == col)
			continue;
		    bseg_get(&asp, &direction, r, c);
		    if (direction == drain[rr][cc]) {
			thisdir = updrain[rr][cc];
			switch (haf_basin_side
				(oldupdir, (int) downdir, thisdir)) {
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
	if (leftflag > riteflag) {
	    value = basin_num - 1;
	    cseg_put(&haf, &value, row, col);
	}
	else {
	    cseg_put(&haf, &basin_num, row, col);
	}
	if (arm_flag) {
	    if (sides == 8) {
		if (new_r[1] != row && new_c[1] != col)
		    stream_length += diag;
		else if (new_r[1] != row)
		    stream_length += window.ns_res;
		else
		    stream_length += window.ew_res;
	    }
	    else {			/* sides == 4 */

		bseg_get(&asp, &asp_value, row, col);
		if (asp_value < 0)
		    asp_value = -asp_value;
		if (asp_value == 2 || asp_value == 6) {
		    if (new_r[1] != row)
			stream_length += window.ns_res;
		    else
			stream_length += diag;
		}
		else {		/* asp_value == 4, 8 */

		    if (new_c[1] != col)
			stream_length += window.ew_res;
		    else
			stream_length += diag;
		}
	    }
	}
	row = new_r[1];
	col = new_c[1];
    }
}
