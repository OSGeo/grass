
/**
 * \file lib/segment/get.c
 *
 * \brief Get segment routines.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2005-2009
 */

#include <string.h>
#include "local_proto.h"


/*bugfix: buf: char* vs int* -> wrong pointer arithmetics!!!. Pierre de Mouveaux - 09 april 2000 */
/* int segment_get (SEGMENT *SEG, register int *buf,int row,int col) */


/**
 * \fn int segment_get (SEGMENT *SEG, void *buf, int row, int col)
 *
 * \brief Get value from segment file.
 *
 * Provides random read access to the segmented data. It gets
 * <i>len</i> bytes of data into <b>buf</b> from the segment file
 * <b>seg</b> for the corresponding <b>row</b> and <b>col</b> in the
 * original data matrix.
 *
 * \param[in] seg segment
 * \param[in,out] buf value return buffer
 * \param[in] row
 * \param[in] col
 * \return 1 of successful
 * \return -1 if unable to seek or read segment file
 */

int segment_get(SEGMENT * SEG, void *buf, off_t row, off_t col)
{
    int index, n, i;

    SEG->segment_address(SEG, row, col, &n, &index);
    if ((i = segment_pagein(SEG, n)) < 0)
	return -1;

    memcpy(buf, &SEG->scb[i].buf[index], SEG->len);

    return 1;
}
