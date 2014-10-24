
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

#include "local_proto.h"

#define SEG_N_ROW_NONZERO(SEG, row, col) \
    (((row) >> (SEG)->srowbits) * (SEG)->spr + ((col) >> (SEG)->scolbits))

#define SEG_INDEX_ROW_NONZERO(SEG, row, col) \
    ((((row) & ((SEG)->srows - 1)) << (SEG)->scolbits) + ((col) & ((SEG)->scols - 1)))

#define SEG_N_ROW_ZERO(SEG, col)	((col) >> (SEG)->scolbits)

#define SEG_INDEX_ROW_ZERO(SEG, col) ((col) & ((SEG)->scols - 1))

#define INDEX_ADJ(SEG, i) \
    ((SEG)->fast_seek ? ((i) << (SEG)->lenbits) : ((i) * (SEG)->len))

int seg_address_fast(const SEGMENT * SEG, off_t row, off_t col, int *n,
			 int *index)
{

#if 1
    if (row) {
	*n = SEG_N_ROW_NONZERO(SEG, row, col);
	*index = INDEX_ADJ(SEG, SEG_INDEX_ROW_NONZERO(SEG, row, col));
    }
    /* for simple arrays */
    else {
	*n = SEG_N_ROW_ZERO(SEG, col);
	*index = INDEX_ADJ(SEG, SEG_INDEX_ROW_ZERO(SEG, col));
    }
#else
    if (row) {
	*n = (row >> SEG->srowbits) * SEG->spr + (col >> SEG->scolbits);
	*index = ((row & (SEG->srows - 1)) << SEG->scolbits) + (col & (SEG->scols - 1));

	/* slower version for testing */
	/*
	off_t seg_r = row >> SEG->srowbits;
	off_t seg_c = col >> SEG->scolbits;

	*n = seg_r * SEG->spr + seg_c;
	
	*index = ((row - (seg_r << SEG->srowbits)) << SEG->scolbits) +
	         col - (seg_c << SEG->scolbits);
	*/
    }
    /* for simple arrays */
    else {
	*n = col >> SEG->scolbits;
	*index = col - ((*n) << SEG->scolbits);
    }

    *index = SEG->fast_seek ? (*index << SEG->lenbits) : (*index * SEG->len);

#endif

    return 0;
}

int seg_address_slow(const SEGMENT * SEG, off_t row, off_t col, int *n,
			 int *index)
{
    if (row) {
	off_t seg_r = row / SEG->srows;
	off_t seg_c = col / SEG->scols;

	*n = seg_r * SEG->spr + seg_c;
	*index = (row - seg_r * SEG->srows) * SEG->scols + col -
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

/**
 * \brief Internal use only
 * 
 * Gets segment address and sets<b>n</b> and <b>index</b>.
 *
 * \param[in] SEG segment
 * \param[in] row
 * \param[in] col
 * \param[in,out] n
 * \param[in,out] index
 * \return always returns 0
 */

int seg_address(const SEGMENT * SEG, off_t row, off_t col, int *n, int *index)
{
    /* old code
     *n = row / SEG->srows * SEG->spr + col / SEG->scols;
     *index = (row % SEG->srows * SEG->scols + col % SEG->scols) * SEG->len;
     */

    /* this function is called at least once every time data are accessed in SEG
     * avoid very slow modulus and divisions, modulus was the main time killer */

    return SEG->address(SEG, row, col, n, index);
}
