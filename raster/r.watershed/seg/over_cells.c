#include "Gwater.h"
#define BIGNEG	-9999999

int
overland_cells_recursive(int row, int col, CELL basin_num, CELL haf_num, CELL * hih_ele)
{
    int r, rr, c, cc;
    CELL new_ele, new_max_ele, value;

    cseg_put(&bas, &basin_num, row, col);
    cseg_put(&haf, &haf_num, row, col);
    new_max_ele = BIGNEG;
    for (r = row - 1, rr = 0; r <= row + 1; r++, rr++) {
	for (c = col - 1, cc = 0; c <= col + 1; c++, cc++) {
	    if (r >= 0 && c >= 0 && r < nrows && c < ncols) {
		cseg_get(&asp, &value, r, c);
		if (value == drain[rr][cc]) {
		    overland_cells(r, c, basin_num, haf_num, &new_ele);
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

/* non-recursive version */
int
overland_cells(int row, int col, CELL basin_num, CELL haf_num, CELL * hih_ele)
{
    int r, rr, c, cc, next_r, next_c;
    CELL value;
    OC_STACK cur;
    int top = 0;

    /* put root on stack */
    ocs[top].row = row;
    ocs[top].col = col;
    cseg_put(&bas, &basin_num, row, col);
    cseg_put(&haf, &haf_num, row, col);

    top++;

    while (top) {
	/* assign basin numbers */
	cur = ocs[--top];
	next_r = cur.row;
	next_c = cur.col;

	/* add all neighbours pouring into current cell to stack */
	for (r = next_r - 1, rr = 0; r <= next_r + 1; r++, rr++) {
	    for (c = next_c - 1, cc = 0; c <= next_c + 1; c++, cc++) {
		if (r >= 0 && c >= 0 && r < nrows && c < ncols) {
		    cseg_get(&asp, &value, r, c);
		    if (value == drain[rr][cc]) {
			if (top >= ocs_alloced) {
			    ocs_alloced += bas_thres;
			    ocs = (OC_STACK *)G_realloc(ocs, ocs_alloced * sizeof(OC_STACK));
			}
			ocs[top].row = r;
			ocs[top].col = c;
			cseg_put(&bas, &basin_num, r, c);
			cseg_put(&haf, &haf_num, r, c);
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
