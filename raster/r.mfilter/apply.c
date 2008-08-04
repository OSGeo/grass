#include "filter.h"

/**************************************************************
 * apply_filter: apply the filter to a single neighborhood
 *
 *  filter:    filter to be applied
 *  input:     input buffers
 **************************************************************/
CELL apply_filter(FILTER * filter, CELL ** input)
{
    int **matrix;
    int size;
    int divisor;
    int round;
    register int r, c;
    register CELL v;

    size = filter->size;
    divisor = filter->divisor;
    matrix = filter->matrix;

    v = 0;
    for (r = 0; r < size; r++)
	for (c = 0; c < size; c++)
	    v += input[r][c] * matrix[r][c];
    /* zero divisor means compute a divisor. this is done by adding the
       numbers is the divisor matrix where the corresponding cell value
       is not zero.
     */
    if (divisor == 0) {
	matrix = filter->dmatrix;
	for (r = 0; r < size; r++)
	    for (c = 0; c < size; c++)
		if (input[r][c])
		    divisor += matrix[r][c];
    }

    /* now round the result to nearest integer. negative numbers are rounded
       a little differently than non-negative numbers
     */
    if (divisor) {
	if (round = divisor / 2) {
	    if ((round > 0 && v > 0) || (round < 0 && v < 0))
		v += round;
	    else
		v -= round;
	}
	v /= divisor;
    }
    else
	v = 0;

    return v;
}
