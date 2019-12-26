
/**
 * \file lib/segment/put.c
 *
 * \brief Segment write routines.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2005-2018
 */

#include <string.h>
#include <grass/gis.h>
#include "local_proto.h"


/*bugfix: buf: char* vs int* -> wrong pointer arithmetics!!!. Pierre de Mouveaux - 09 april 2000 */
/* int Segment_put (SEGMENT *SEG,int *buf,int row,int col) */


/**
 * \fn int Segment_put (SEGMENT *SEG, void *buf, int row, int col)
 *
 * \brief Write value to segment file.
 *
 * Provides random write access to the segmented data. It
 * copies <i>len</i> bytes of data from <b>buf</b> into the segment
 * structure <b>seg</b> for the corresponding <b>row</b> and <b>col</b> in
 * the original data matrix.
 *
 * The data is not written to disk immediately. It is stored in a memory segment
 * until the segment routines decide to page the segment to disk.
 *
 * \param[in,out] seg segment
 * \param[in] buf value to write to segment
 * \param[in] row
 * \param[in] col
 * \return 1 if successful
 * \return -1 if unable to seek or write segment file
 */

int Segment_put(SEGMENT * SEG, const void *buf, off_t row, off_t col)
{
    int index, n, i;

    if (SEG->cache) {
	memcpy(SEG->cache + ((size_t)row * SEG->ncols + col) * SEG->len, buf, SEG->len);
	
	return 1;
    }

    SEG->address(SEG, row, col, &n, &index);
    if ((i = seg_pagein(SEG, n)) < 0) {
	G_warning("segment lib: put: pagein failed");
	return -1;
    }

    SEG->scb[i].dirty = 1;

    memcpy(&SEG->scb[i].buf[index], buf, SEG->len);

    return 1;
}
