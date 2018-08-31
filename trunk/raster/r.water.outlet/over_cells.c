#include "basin.h"

#define INCR	64
#define ONE_CELL struct one_cell
ONE_CELL {
    int r, c;
};

int overland_cells(int row, int col)
{
    int r, rr, c, cc, num_cells, size_more;
    CELL value;
    ONE_CELL *Acells;

    if (nrows > ncols)
	size_more = nrows;
    else
	size_more = ncols;
    Acells = (ONE_CELL *) G_malloc(size_more * sizeof(ONE_CELL));
    num_cells = 1;
    Acells[0].r = row;
    Acells[0].c = col;
    while (num_cells--) {
	row = Acells[num_cells].r;
	col = Acells[num_cells].c;
	bas[SEG_INDEX(ba_seg, row, col)] = 1;
	for (r = row - 1, rr = 0; r <= row + 1; r++, rr++) {
	    for (c = col - 1, cc = 0; c <= col + 1; c++, cc++) {
		if (r >= 0 && c >= 0 && r < nrows && c < ncols) {
		    value = drain_ptrs[SEG_INDEX(pt_seg, r, c)];
		    if ((value == drain[rr][cc]) &&
			(bas[SEG_INDEX(ba_seg, r, c)] == 0)) {
			if (num_cells == size_more) {
			    size_more += INCR;
			    Acells = (ONE_CELL *) G_realloc(Acells,
							    size_more *
							    sizeof(ONE_CELL));
			}
			Acells[num_cells].r = r;
			Acells[num_cells++].c = c;
		    }
		}
	    }
	}
    }

    return 0;
}
