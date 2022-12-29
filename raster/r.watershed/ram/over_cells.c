#include "Gwater.h"
#define BIGNEG	-9999999

int
overland_cells_recursive(int row, int col, CELL basin_num, CELL haf_num,
			 CELL * hih_ele)
{
    int r, rr, c, cc;
    CELL new_ele, new_max_ele, value;

    bas[SEG_INDEX(bas_seg, row, col)] = basin_num;
    haf[SEG_INDEX(haf_seg, row, col)] = haf_num;
    new_max_ele = BIGNEG;
    for (r = row - 1, rr = 0; r <= row + 1; r++, rr++) {
	for (c = col - 1, cc = 0; c <= col + 1; c++, cc++) {
	    if (r >= 0 && c >= 0 && r < nrows && c < ncols) {
		if (r == row && c == col)
		    continue;
		value = asp[SEG_INDEX(asp_seg, r, c)];
		if (value == drain[rr][cc]) {
		    overland_cells(r, c, basin_num, haf_num, &new_ele);
		}
	    }
	}
    }
    /*
       if (arm_flag) {
       if (new_max_ele == BIGNEG) {
       *hih_ele = alt[SEG_INDEX(alt_seg, row, col)];
       }
       else {
       *hih_ele = new_max_ele;
       }
       }
     */

    return 0;
}

/* non-recursive version */
int
overland_cells(int row, int col, CELL basin_num, CELL haf_num, CELL * hih_ele)
{
    int r, c, rr, cc, next_r, next_c;
    int top = 0, idx;

    /* put root on stack */
    ocs[top].row = row;
    ocs[top].col = col;
    idx = SEG_INDEX(bas_seg, row, col);
    bas[idx] = basin_num;
    haf[idx] = haf_num;

    top++;

    while (top) {
	top--;
	next_r = ocs[top].row;
	next_c = ocs[top].col;

	for (r = next_r - 1, rr = 0; r <= next_r + 1; r++, rr++) {
	    for (c = next_c - 1, cc = 0; c <= next_c + 1; c++, cc++) {
		if (r >= 0 && c >= 0 && r < nrows && c < ncols) {
		    if (r == row && c == col)
			continue;
		    idx = SEG_INDEX(bas_seg, r, c);
		    if (asp[idx] == drain[rr][cc]) {
			if (top >= ocs_alloced) {
			    ocs_alloced += bas_thres;
			    ocs =
				(OC_STACK *) G_realloc(ocs,
						       ocs_alloced *
						       sizeof(OC_STACK));
			}
			ocs[top].row = r;
			ocs[top].col = c;
			bas[idx] = basin_num;
			haf[idx] = haf_num;

			top++;
		    }
		}
	    }
	}
    }

    /*
       if (arm_flag) {
       if (new_max_ele == BIGNEG) {
       cseg_get(&alt, hih_ele, row, col);
       }
       else {
       *hih_ele = new_max_ele;
       }
       }
     */

    return 0;
}
