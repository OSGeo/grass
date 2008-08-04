
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
 * \date 2005-2006
 */

#include <grass/segment.h>


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
    *n = row / SEG->srows * SEG->spr + col / SEG->scols;
    *index = (row % SEG->srows * SEG->scols + col % SEG->scols) * SEG->len;

    return 0;
}
