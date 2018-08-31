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
