
/**
 * \file debug.c
 *
 * \brief Segment debug routines.
 *
 * This file has debug versions of <i>segment_get()</i> and 
 * <i>segment_put()</i> which check the row,col and print error messages 
 * to <em>stderr</em> upon violations.
 *
 * <b>Build Note:</b> Load the debug.o file before the SEGMENTLIB and 
 * the debug versions will supercede the original.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2005-2006
 */

#include <stdio.h>
#include <string.h>
#include <grass/segment.h>


static int check(const SEGMENT *, int, int, char *);


/**
 * \fn int segment_get (SEGMENT *SEG, void *buf, int row, int col)
 *
 * \brief Get value from segment file.
 *
 * Provides random read access to the segmented data. It gets
 * <i>len</i> bytes of data into <b>value</b> from the segment file
 * <b>seg</b> for the corresponding <b>row</b> and <b>col</b> in the
 * original data matrix.
 *
 * \param[in] seg
 * \param[in,out] buf
 * \param[in] row
 * \param[in] col
 * \return 1 on success
 * \return -1 if unable to seek or read segment file
 */

int segment_get(SEGMENT * SEG, void *buf, int row, int col)
{
    int index, n;

    if (!check(SEG, row, col, "segment_get"))
	return -1;

    segment_address(SEG, row, col, &n, &index);
    if ((i = segment_pagein(SEG, n)) < 0)
	return -1;

    memcpy(buf, &SEG->scb[i].buf[index], SEG->len);

    return 1;
}


/**
 * \fn int segment_put (SEGMENT *SEG, void *buf, int row, int col)
 *
 * \brief Put value to segment file.
 *
 * Provides random write access to the segmented data. It
 * copies <i>len</i> bytes of data from <b>value</b> into the segment
 * structure <b>seg</b> for the corresponding <b>row</b> and <b>col</b> in
 * the original data matrix.
 *
 * The data is not written to disk immediately. It is stored in a memory segment
 * until the segment routines decide to page the segment to disk.
 *
 * \param[in] seg
 * \param[in,out] buf
 * \param[in] row
 * \param[in] col
 * \return 1 on success
 * \return -1 if unable to seek or write segment file
 */

int segment_put(SEGMENT * SEG, const void *buf, int row, int col)
{
    int index, n;

    if (!check(SEG, row, col, "segment_put"))
	return -1;

    segment_address(SEG, row, col, &n, &index);
    if ((i = segment_pagein(SEG, n)) < 0)
	return -1;

    SEG->scb[i].dirty = 1;

    memcpy(&SEG->scb[i].buf[index], buf, SEG->len);

    return 1;
}


static int check(const SEGMENT * SEG, int row, int col, char *me)
{
    int r = row >= 0 && row < SEG->nrows;
    int c = col >= 0 && col < SEG->ncols;

    if (r && c)
	return 1;

    fprintf(stderr, "%s(SEG=%lx,fd=%d,row=%d,col=%d) ",
	    me, (unsigned long int)SEG, SEG->fd, row, col);
    if (!r) {
	fprintf(stderr, "bad row ");
	if (row >= SEG->nrows)
	    fprintf(stderr, "(max %d) ", SEG->nrows - 1);
    }
    if (!c) {
	fprintf(stderr, "bad col ");
	if (col >= SEG->ncols)
	    fprintf(stderr, "(max %d) ", SEG->ncols - 1);
    }
    fprintf(stderr, "\n");

    return 0;
}
