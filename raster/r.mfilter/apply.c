#include <grass/raster.h>
#include "filter.h"

/**************************************************************
 * apply_filter: apply the filter to a single neighborhood
 *
 *  filter:    filter to be applied
 *  input:     input buffers
 **************************************************************/
DCELL apply_filter(FILTER * filter, DCELL ** input)
{
    int size = filter->size;
    double **matrix = filter->matrix;
    double divisor = filter->divisor;
    int r, c;
    DCELL v;

    v = 0;

    if (divisor == 0) {
	int have_result = 0;

	for (r = 0; r < size; r++)
	    for (c = 0; c < size; c++) {
		if (Rast_is_d_null_value(&input[r][c]))
		    continue;
		v += input[r][c] * matrix[r][c];
		divisor += filter->dmatrix[r][c];
		have_result = 1;
	    }

	if (have_result)
	    v /= divisor;
	else
	    Rast_set_d_null_value(&v, 1);
    }
    else {
	for (r = 0; r < size; r++)
	    for (c = 0; c < size; c++) {
		if (Rast_is_d_null_value(&input[r][c])) {
		    Rast_set_d_null_value(&v, 1);
		    return v;
		}
		v += input[r][c] * matrix[r][c];
	    }

	v /= divisor;
    }

    return v;
}
