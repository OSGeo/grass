#include <unistd.h>
#include <stdlib.h>

#include <grass/config.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "R.h"

/*!

   <h3>GRASS Raster Format</h3>

   Small example to illustrate the raster format:

   A file may contain the following 3x3 floating point matrix:
   \verbatim
   10.000 20.000 30.000
   20.000 40.000 50.000
   30.000 50.000 60.000
   \endverbatim

   The header is a single byte, equal to sizeof(off_t) (typically 4 on a
   32-bit platform, 8 on a 64-bit platform). Then, NROWS+1 offsets are
   written as off_t's (i.e. 4 or 8 bytes, depending upon platform) in
   big-endian (Motorola) byte order.
   <P>
   Thus, above example is actually interpreted as:
   \verbatim
   4               sizeof(off_t)
   0 0 0 17        offset of row 0
   0 0 0 36        offset of row 1
   0 0 0 55        offset of row 2
   0 0 0 74        offset of end of data
   \endverbatim

   See Rast__write_row_ptrs() below for the code which writes this data. 
   However, note that the row offsets are initially zero; 
   they get overwritten later (if you are writing compressed data,
   you don't know how much space it will require until you've compressed
   it).

   As for the format of the actual row data, see put_fp_data() in
   src/libes/gis/put_row.c and RFC 1014 (the XDR specification):
   http://www.faqs.org/rfcs/rfc1014.html

 */

/**********************************************************************
 *
 *   Rast__check_format(int fd)
 *
 *   Check to see if map with file descriptor "fd" is in compressed
 *   format.   If it is, the offset table at the beginning of the 
 *   file (which gives seek addresses into the file where code for
 *   each row is found) is read into the File Control Buffer (FCB).
 *   The compressed flag in the FCB is appropriately set.
 *
 *   returns:    1 if row pointers were read successfully, -1 otherwise
 **********************************************************************/

int Rast__check_format(int fd)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    unsigned char compress[4];

    /*
     * Check to see if the file is in compress mode
     * 4 possibilities
     *   compressed flag in cellhd is negative (meaning pre 3.0 cell file)
     *       compression flag is first 3 bytes of cell file
     *   compression flag is 0 - not compressed
     *   compression flag is 1 - compressed using RLE (int) or zlib (FP)
     *   compression flag is 2 - compressed using zlib
     */

    if (fcb->cellhd.compressed < 0) {
	if (read(fcb->data_fd, compress, 3) != 3
	    || compress[0] != 251 || compress[1] != 255 || compress[2] != 251)
	    fcb->cellhd.compressed = 0;
    }

    if (!fcb->cellhd.compressed)
	return 1;

    /* allocate space to hold the row address array */
    fcb->row_ptr = G_calloc(fcb->cellhd.rows + 1, sizeof(off_t));

    /* read the row address array */
    return Rast__read_row_ptrs(fd);
}

static int read_row_ptrs(int nrows, int old, off_t *row_ptr, int fd)
{
    unsigned char nbytes;
    unsigned char *buf, *b;
    int n;
    int row;

    /*
     * pre3.0 row addresses were written directly from the array of off_t's
     * (this makes them machine dependent)
     */

    if (old) {
	n = (nrows + 1) * sizeof(off_t);
	if (read(fd, row_ptr, n) != n)
	    goto badread;
	return 1;
    }

    /*
     * 3.0 row address array is in a machine independent format
     * (warning - the format will work even if the sizeof(off_t) is
     *  not the same from machine to machine, as long as the
     *  actual values do not exceed the capability of the off_t)
     */

    if (read(fd, &nbytes, 1) != 1)
	goto badread;
    if (nbytes == 0)
	goto badread;

    n = (nrows + 1) * nbytes;
    buf = G_malloc(n);
    if (read(fd, buf, n) != n)
	goto badread;

    for (row = 0, b = buf; row <= nrows; row++) {
	off_t v = 0;

	for (n = 0; n < (int)nbytes; n++) {
	    unsigned char c = *b++;

	    if (nbytes > sizeof(off_t) && n < nbytes - sizeof(off_t) &&
		c != 0)
		goto badread;

	    v <<= 8;
	    v += c;
	}

	row_ptr[row] = v;
    }

    G_free(buf);

    return 1;

  badread:
    return -1;
}

int Rast__read_row_ptrs(int fd)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int nrows = fcb->cellhd.rows;
    int old = fcb->cellhd.compressed < 0;

    if (read_row_ptrs(nrows, old, fcb->row_ptr, fcb->data_fd) < 0) {
	G_warning(_("Fail of initial read of compressed file [%s in %s]"),
		  fcb->name, fcb->mapset);
	return -1;
    }

    return 1;
}

int Rast__read_null_row_ptrs(int fd, int null_fd)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int nrows = fcb->cellhd.rows;

    if (read_row_ptrs(nrows, 0, fcb->null_row_ptr, null_fd) < 0) {
	G_warning(_("Fail of initial read of compressed null file [%s in %s]"),
		  fcb->name, fcb->mapset);
	return -1;
    }

    return 1;
}

static int write_row_ptrs(int nrows, off_t *row_ptr, int fd)
{
    int nbytes = sizeof(off_t);
    unsigned char *buf, *b;
    int len, row, result;

    lseek(fd, 0L, SEEK_SET);

    len = (nrows + 1) * nbytes + 1;
    b = buf = G_malloc(len);
    *b++ = nbytes;

    for (row = 0; row <= nrows; row++) {
	off_t v = row_ptr[row];
	int i;

	for (i = nbytes - 1; i >= 0; i--) {
	    b[i] = v & 0xff;
	    v >>= 8;
	}

	b += nbytes;
    }

    result = (write(fd, buf, len) == len);
    G_free(buf);

    return result;
}

int Rast__write_row_ptrs(int fd)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int nrows = fcb->cellhd.rows;

    return write_row_ptrs(nrows, fcb->row_ptr, fcb->data_fd);
}

int Rast__write_null_row_ptrs(int fd, int null_fd)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int nrows = fcb->cellhd.rows;

    return write_row_ptrs(nrows, fcb->null_row_ptr, null_fd);
}
