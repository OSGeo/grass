
/**
 * \file format.c
 *
 * \brief Segment formatting routines.
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
#include <limits.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/segment.h>


static int _segment_format(int, int, int, int, int, int, int);
static int write_int(int, int);
static int zero_fill(int, off_t);

/* fd must be open for write */


/**
 * \fn int segment_format (int fd, int nrows, int ncols, int srows, int scols, int len)
 *
 * \brief Format a segment file.
 *
 * The segmentation routines require a disk file to be used for paging 
 * segments in and out of memory. This routine formats the file open for 
 * write on file descriptor <b>fd</b> for use as a segment file.
 *
 * A segment file must be formatted before it can be processed by other 
 * segment routines. The configuration parameters <b>nrows</b>, 
 * <b>ncols</b>, <b>srows</b>, <b>scols</b>, and <b>len</b> are written 
 * to the beginning of the segment file which is then filled with zeros.
 *
 * The corresponding nonsegmented data matrix, which is to be 
 * transferred to the segment file, is <b>nrows</b> by <b>ncols</b>. The 
 * segment file is to be formed of segments which are <b>srows</b> by 
 * <b>scols</b>. The data items have length <b>len</b> bytes. For 
 * example, if the <em>data type is int</em>, <em>len is sizeof(int)</em>.
 *
 * \param[in] fd file descriptor
 * \param[in] nrows number of non-segmented rows
 * \param[in] ncols number of non-segmented columns
 * \param[in] srows segment rows
 * \param[in] scols segment columns
 * \param[in] len length of data type
 * \return 1 of successful
 * \return -1 if unable to seek or write <b>fd</b>
 * \return -3 if illegal parameters are passed
 */

int segment_format(int fd, int nrows, int ncols, int srows, int scols,
		   int len)
{
    return _segment_format(fd, nrows, ncols, srows, scols, len, 1);
}

/**
 * \fn int segment_format_nofill (int fd, int nrows, int ncols, int srows, int scols, int len)
 *
 * \brief Format a segment file.
 *
 * The segmentation routines require a disk file to be used for paging 
 * segments in and out of memory. This routine formats the file open for 
 * write on file descriptor <b>fd</b> for use as a segment file.
 *
 * A segment file must be formatted before it can be processed by other 
 * segment routines. The configuration parameters <b>nrows</b>, 
 * <b>ncols</b>, <b>srows</b>, <b>scols</b>, and <b>len</b> are written 
 * to the beginning of the segment file which is then filled with zeros.
 *
 * The corresponding nonsegmented data matrix, which is to be 
 * transferred to the segment file, is <b>nrows</b> by <b>ncols</b>. The 
 * segment file is to be formed of segments which are <b>srows</b> by 
 * <b>scols</b>. The data items have length <b>len</b> bytes. For 
 * example, if the <em>data type is int</em>, <em>len is sizeof(int)</em>.
 *
 * <b>Note:</b> This version of the function does <b>not</b> fill in the 
 * initialized data structures with zeros.
 *
 * \param[in] fd file descriptor
 * \param[in] nrows number of non-segmented rows
 * \param[in] ncols number of non-segmented columns
 * \param[in] srows segment rows
 * \param[in] scols segment columns
 * \param[in] len length of data type
 * \return 1 of successful
 * \return -1 if unable to seek or write <b>fd</b>
 * \return -3 if illegal parameters are passed
 */

int segment_format_nofill(int fd, int nrows, int ncols, int srows, int scols,
			  int len)
{
    return _segment_format(fd, nrows, ncols, srows, scols, len, 0);
}


static int _segment_format(int fd,
			   int nrows, int ncols,
			   int srows, int scols, int len, int fill)
{
    off_t nbytes;
    int spr, size;

    if (nrows <= 0 || ncols <= 0 || len <= 0 || srows <= 0 || scols <= 0) {
	G_warning("segment_format(fd,%d,%d,%d,%d,%d): illegal value(s)",
		  nrows, ncols, srows, scols, len);
	return -3;
    }

    if (sizeof(off_t) == 4 && sizeof(double) >= 8) {
	double d_size;
	off_t o_size;

	spr = ncols / scols;
	if (ncols % scols)
	    spr++;

	/* calculate total number of segments */
	d_size = (double) spr * ((nrows + srows - 1) / srows);
	/* multiply with segment size */
	d_size *= srows * scols * len;

	/* add header */
	d_size += 5 * sizeof(int);

	o_size = (off_t) d_size;

	/* this test assumes that all off_t values can be exactly 
	 * represented as double if sizeof(off_t) = 4 and sizeof(double) >= 8 */
	if ((double) o_size != d_size) {
	    G_warning(_("Segment format: file size too large"));
	    G_warning(_("Please recompile with Large File Support (LFS)"));
	    return -1;
	}
    }

    if (lseek(fd, 0L, SEEK_SET) == (off_t) -1) {
	G_warning("segment_format(): Unable to seek (%s)", strerror(errno));
	return -1;
    }

    if (!write_int(fd, nrows) || !write_int(fd, ncols)
	|| !write_int(fd, srows) || !write_int(fd, scols)
	|| !write_int(fd, len))
	return -1;

    if (!fill)
	return 1;

    spr = ncols / scols;
    if (ncols % scols)
	spr++;

    size = srows * scols * len;

    /* calculate total number of segments */
    nbytes = spr * ((nrows + srows - 1) / srows);
    nbytes *= size;

    /* fill segment file with zeros */
    /* NOTE: this could be done faster using lseek() by seeking
     * ahead nbytes and then writing a single byte of 0,
     * provided lseek() on all version of UNIX will create a file
     * with holes that read as zeros.
     */
    if (zero_fill(fd, nbytes) < 0)
	return -1;

    return 1;
}


static int write_int(int fd, int n)
{
    if (write(fd, &n, sizeof(int)) != sizeof(int)) {
	G_warning("segment_format(): Unable to write (%s)", strerror(errno));
	return 0;
    }

    return 1;
}


static int zero_fill(int fd, off_t nbytes)
{
#ifndef USE_LSEEK
    char buf[16384];
    register char *b;
    register int n;

    /* zero buf */
    n = nbytes > sizeof(buf) ? sizeof(buf) : nbytes;
    b = buf;
    while (n-- > 0)
	*b++ = 0;

    while (nbytes > 0) {
	n = nbytes > sizeof(buf) ? sizeof(buf) : nbytes;
	if (write(fd, buf, n) != n) {
	    G_warning("segment zero_fill(): Unable to write (%s)", strerror(errno));
	    return -1;
	}
	nbytes -= n;
    }
    return 1;
#else
    /* Using lseek (faster upon initialization).
       NOTE: This version doesn't allocate disk storage for the file; storage will
       be allocated dynamically as blocks are actually written. This could 
       result in zero_fill() succeeding but a subsequent call to write() failing
       with ENOSPC ("No space left on device").
     */

    static const char buf[10];

    G_debug(3, "Using new segmentation code...");
    if (lseek(fd, nbytes - 1, SEEK_CUR) < 0) {
	G_warning("segment zero_fill(): Unable to seek (%s)", strerror(errno));
	return -1;
    }
    if (write(fd, buf, 1) != 1) {
	G_warning("segment zero_fill(): Unable to write (%s)", strerror(errno));
	return -1;
    }

    return 1;
#endif
}
