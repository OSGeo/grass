
/**
 * \file get_row.c
 *
 * \brief Segment row retrieval routines.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2005-2009
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <grass/gis.h>
#include <grass/segment.h>


/**
 * \fn int segment_get_row (SEGMENT *SEG, void *buf, int row)
 *
 * \brief Read row from segment file.
 *
 * Transfers data from a segment file, row by row, into memory
 * (which can then be written to a regular matrix file). <b>Seg</b> is the
 * segment structure that was configured from a call to 
 * <i>segment_init()</i>.
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

int segment_get_row(const SEGMENT * SEG, void *buf, int row)
{
    int size;
    int ncols;
    int scols;
    int n, index, col;

    ncols = SEG->ncols - SEG->spill;
    scols = SEG->scols;
    size = scols * SEG->len;

    for (col = 0; col < ncols; col += scols) {
	segment_address(SEG, row, col, &n, &index);
	if (segment_seek(SEG, n, index) < 0)
	    return -1;

	if (read(SEG->fd, buf, size) != size) {
	    G_warning("segment_get_row: %s", strerror(errno));
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
	segment_address(SEG, row, col, &n, &index);
	if (segment_seek(SEG, n, index) < 0)
	    return -1;

	if (read(SEG->fd, buf, size) != size) {
	    G_warning("segment_get_row: %s", strerror(errno));
	    return -1;
	}
    }

    return 1;
}
