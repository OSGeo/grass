#include <stdio.h>
#include <stdlib.h>
#include <grass/rowio.h>


/*!
 * \brief configure rowio structure
 *
 * Rowio_setup()
 * initializes the ROWIO structure <b>r</b> and allocates the required memory
 * buffers. The file descriptor <b>fd</b> must be open for reading. The number
 * of rows to be held in memory is <b>nrows.</b> The length in bytes of each
 * row is <b>len.</b> The routine which will be called to read data from the
 * file is <b>getrow()</b> and must be provided by the programmer. If the
 * application requires that the rows be written back into the file if changed,
 * the file descriptor <b>fd</b> must be open for write as well, and the
 * programmer must provide a <b>putrow()</b> routine to write the data into
 * the file. If no writing of the file is to occur, specify NULL for
 * <b>putrow().</b>
 * Return codes:
 * 1 ok 
 * -1 there is not enough memory for buffer allocation
 *
 *  \param 
 *  \return int
 */

int rowio_setup(ROWIO * R,
		int fd, int nrows, int len,
		int (*getrow) (int, void *, int, int),
		int (*putrow) (int, const void *, int, int))
{
    int i;

    R->getrow = getrow;
    R->putrow = putrow;
    R->nrows = nrows;
    R->len = len;
    R->cur = -1;
    R->buf = NULL;
    R->fd = fd;

    R->rcb = (struct ROWIO_RCB *)malloc(nrows * sizeof(struct ROWIO_RCB));
    if (R->rcb == NULL) {
	fprintf(stderr, "rowio_setup: out of memory\n");
	return -1;
    }
    for (i = 0; i < nrows; i++) {
	R->rcb[i].buf = malloc(len);
	if (R->rcb[i].buf == NULL) {
	    fprintf(stderr, "rowio_setup: out of memory\n");
	    return -1;
	}
	R->rcb[i].row = -1;	/* mark not used */
    }
    return 1;
}
