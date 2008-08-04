#include <stdio.h>
#include <string.h>
#include <grass/rowio.h>


/*!
 * \brief write a row
 *
 * Rowio_put()
 * writes the buffer <b>buf</b>, which holds the data for row <b>n</b>, into
 * the ROWIO structure <b>r.</b> If the row requested is currently in memory,
 * the buffer is simply copied into the structure and marked as having been
 * changed. It will be written out later. Otherwise it is written immediately.
 * Note that when the row is finally written to disk, the <b>putrow()</b>
 * routine specified in <i>rowio_setup</i> is called to write row <b>n</b>
 * to the file. <b>rowio_flush</b> ( r) force pending updates to disk ROWIO *r;
 * Rowio_flush() forces all rows modified by <i>rowio_put</i> to be written
 * to the file. This routine must be called before closing the file or releasing
 * the rowio structure if rowio_put() has been called.
 *
 *  \param r
 *  \param buf
 *  \param n
 *  \return int
 */

int rowio_put(ROWIO * R, const void *buf, int row)
{
    int i;

    if (row < 0)
	return 0;

    for (i = 0; i < R->nrows; i++)
	if (row == R->rcb[i].row) {
	    memcpy(R->rcb[i].buf, buf, R->len);
	    R->rcb[i].dirty = 1;
	    return 1;
	}
    return ((*R->putrow) (R->fd, buf, row, R->len));
}
