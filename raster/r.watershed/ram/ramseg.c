#include <stdio.h>
#include "ramseg.h"

int size_array(int *ram_seg, int nrows, int ncols)
{
    int size, segs_in_col;

    segs_in_col = ((nrows - 1) >> RAMSEGBITS) + 1;
    *ram_seg = ((ncols - 1) >> RAMSEGBITS) + 1;
    size = ((((nrows - 1) >> RAMSEGBITS) + 1) << RAMSEGBITS) *
	((((ncols - 1) >> RAMSEGBITS) + 1) << RAMSEGBITS);
    size -= ((segs_in_col << RAMSEGBITS) - nrows) << RAMSEGBITS;
    size -= (*ram_seg << RAMSEGBITS) - ncols;
    return (size);
}

/* get r, c from seg_index */
int seg_index_rc(int ramseg, int seg_index, int *r, int *c)
{
    int seg_no, seg_remainder;

    seg_no = seg_index >> DOUBLEBITS;
    seg_remainder = seg_index - (seg_no << DOUBLEBITS);
    *r = ((seg_no / ramseg) << RAMSEGBITS) + (seg_remainder >> RAMSEGBITS);
    *c = ((seg_no - ((*r) >> RAMSEGBITS) * ramseg) << RAMSEGBITS) +
	seg_remainder - (((*r) & SEGLENLESS) << RAMSEGBITS);
    return seg_no;
}
