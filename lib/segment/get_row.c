
/**
 * \file lib/segment/get_row.c
 *
 * \brief Segment row retrieval routines.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2005-2018
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <grass/gis.h>
#include "local_proto.h"


/**
 * \fn int Segment_get_row (SEGMENT *SEG, void *buf, int row)
 *
 * \brief Read row from segment file.
 *
 * Transfers data from a segment file, row by row, into memory
 * (which can then be written to a regular matrix file). <b>Seg</b> is the
 * segment structure that was configured from a call to 
 * <i>Segment_init()</i>.
 *
 * <b>Buf</b> will be filled with <em>ncols*len</em> bytes of data
 * corresponding to the <b>row</b> in the data matrix.
 *
 * \param[in] seg segment
 * \param[in,out] buf
 * \param[in] row
 * \return 1 if successful
 * \return -1 if unable to seek or read segment file
 */

int Segment_get_row(const SEGMENT * SEG, void *buf, off_t row)
{
    int size;
    off_t ncols, col;
    int scols;
    int n, index;

    if (SEG->cache) {
	memcpy(buf, SEG->cache + ((size_t)row * SEG->ncols) * SEG->len, SEG->len * SEG->ncols);
	
	return 1;
    }

    ncols = SEG->ncols - SEG->spill;
    scols = SEG->scols;
    size = scols * SEG->len;

    for (col = 0; col < ncols; col += scols) {
	SEG->address(SEG, row, col, &n, &index);
	SEG->seek(SEG, n, index);

	if (read(SEG->fd, buf, size) != size) {
	    G_warning("Segment_get_row: %s", strerror(errno));
	    return -1;
	}

	/* The buf variable is a void pointer and thus points to anything. */
	/* Therefore, it's size is unknown and thus, it cannot be used for */
	/* pointer arithmetic (some compilers treat this as an error - SGI */
	/* MIPSPro compiler for one). Since the read command is reading in */
	/* "size" bytes, cast the buf variable to char * before incrementing */
	buf = ((char *)buf) + size;
    }
    if ((size = SEG->spill * SEG->len)) {
	SEG->address(SEG, row, col, &n, &index);
	SEG->seek(SEG, n, index);

	if (read(SEG->fd, buf, size) != size) {
	    G_warning("Segment_get_row: %s", strerror(errno));
	    return -1;
	}
    }

    return 1;
}
