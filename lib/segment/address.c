
/**
 * \file address.c
 *
 * \brief Address routines.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2005-2009
 */

#include <grass/segment.h>

int segment_address_fast(const SEGMENT * SEG, int row, int col, int *n,
			 int *index)
{
    if (row) {
	int seg_r = row >> SEG->srowbits;
	int seg_c = col >> SEG->scolbits;

	*n = seg_r * SEG->spr + seg_c;
	*index = ((row - (seg_r << SEG->srowbits)) << SEG->scolbits) +
	         col - (seg_c << SEG->scolbits);

	/*
	*n = (row >> SEG->srowbits) * SEG->spr + (col >> SEG->scolbits);
	*index = ((row & (SEG->srows - 1)) << SEG->scolbits) + (col & (SEG->scols - 1));
	*/
    }
    /* for simple arrays */
    else {
	*n = col >> SEG->scolbits;
	*index = col - ((*n) << SEG->scolbits);
    }
    if (SEG->slow_seek == 0)
	*index = *index << SEG->lenbits;
    else
	*index *= SEG->len;

    return 0;
}

int segment_address_slow(const SEGMENT * SEG, int row, int col, int *n,
			 int *index)
{
    if (row) {
	int seg_r = row / SEG->srows;
	int seg_c = col / SEG->scols;

	*n = seg_r * SEG->spr + seg_c;
	*index =
	    (row - seg_r * SEG->srows) * SEG->scols + col -
	    seg_c * SEG->scols;
    }
    /* for simple arrays */
    else {
	*n = col / SEG->scols;
	*index = col - *n * SEG->scols;
    }
    *index *= SEG->len;

    return 0;
}

static int (*segment_adrs[2]) () = {
segment_address_fast, segment_address_slow};

/**
 * \fn int segment_address (SEGMENT *SEG, int row, int col, int *n, int *index)
 *
 * \brief Gets segment address and returns <b>n</b> and <b>index</b>.
 *
 * \param[in] SEG segment
 * \param[in] row
 * \param[in] col
 * \param[in,out] n
 * \param[in,out] index
 * \return always returns 0
 */

int segment_address(const SEGMENT * SEG, int row, int col, int *n, int *index)
{
    /* old code
     *n = row / SEG->srows * SEG->spr + col / SEG->scols;
     *index = (row % SEG->srows * SEG->scols + col % SEG->scols) * SEG->len;
     */

    /* this function is called at least once every time data are accessed in SEG
     * avoid very slow modulus and divisions, modulus was the main time killer */

    return (*segment_adrs[SEG->slow_adrs]) (SEG, row, col, n, index);
}
