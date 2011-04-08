
/**
 * \file put_row.c
 *
 * \brief Write segment row routines.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2005-2009
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/segment.h>


/*      buf is CELL *   WRAT code       */
/* int segment_put_row (SEGMENT *SEG, CELL *buf,int row) */


/**
 * \fn int segment_put_row (SEGMENT *SEG, void *buf, int row)
 *
 * \brief Write row to segment file.
 *
 * Transfers non-segmented matrix data, row by row, into a segment
 * file.  <b>seg</b> is the segment structure that was configured from a 
 * call to <i>segment_init()</i>. <b>buf</b> should contain 
 * <em>ncols*len</em> bytes of data to be transferred to the segment 
 * file. <b>row</b> specifies the row from the data matrix being 
 * transferred.
 *
 * \param[in,out] seg segment
 * \param[in] buf data to write to segment
 * \param[in] row
 * \return 1 if successful
 * \return -1 if unable to seek or write segment file
 */

int segment_put_row(const SEGMENT * SEG, const void *buf, int row)
{
    int size;
    int ncols;
    int scols;
    int n, index, col;
    int result;

    ncols = SEG->ncols - SEG->spill;
    scols = SEG->scols;
    size = scols * SEG->len;
    /*      printf("segment_put_row ncols: %d, scols %d, size: %d, col %d, row: %d,  SEG->fd: %d\n",ncols,scols,size,col,row, SEG->fd); */

    for (col = 0; col < ncols; col += scols) {
	segment_address(SEG, row, col, &n, &index);
	if (segment_seek(SEG, n, index) < 0) {
	    G_warning
		("Failed seek in segment file for index = %d n = %d at col:row %d:%d",
		 index, n, col, row);
	    return -1;
	}

	if ((result = write(SEG->fd, buf, size)) != size) {
	    G_warning("segment_put_row write error %s", strerror(errno));
	    /*      printf("segment_put_row result = %d. ncols: %d, scols %d, size: %d, col %d, row: %d,  SEG->fd: %d\n",result,ncols,scols,size,col,row, SEG->fd); */
	    return -1;
	}

	/* The buf variable is a void pointer and thus points to anything. */
	/* Therefore, it's size is unknown and thus, it cannot be used for */
	/* pointer arithmetic (some compilers treat this as an error - SGI */
	/* MIPSPro compiler for one). Since the read command is reading in */
	/* "size" bytes, cast the buf variable to char * before incrementing */
	buf = ((const char *)buf) + size;
    }

    if ((size = SEG->spill * SEG->len)) {
	segment_address(SEG, row, col, &n, &index);
	if (segment_seek(SEG, n, index) < 0) {
	    G_warning
		("Failed seek in segment file for index = %d n = %d at col:row %d:%d",
		 index, n, col, row);
	    return -1;
	}
	if (write(SEG->fd, buf, size) != size) {
	    G_warning("segment_put_row final write error: %s",
		      strerror(errno));
	    return -1;
	}
    }

    return 1;
}
