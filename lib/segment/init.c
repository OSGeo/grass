
/****************************************************************************
 *
 * MODULE:       segment
 * AUTHOR(S):    CERL
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Markus Neteler <neteler itc.it>,
 *               Markus Metz <markus.metz.giswork googlemail.com>
 * PURPOSE:      Segment initialization routines
 * COPYRIGHT:    (C) 2000-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <grass/gis.h>
#include "local_proto.h"


static int read_int(int, int *);
static int read_off_t(int, off_t *);

/* fd must be open for read and write */


/**
 * \fn int Segment_init (SEGMENT *SEG, int fd, int nseg)
 *
 * \brief Initialize segment structure.
 *
 * Initializes the <b>seg</b> structure. The file on <b>fd</b> is
 * a segment file created by <i>Segment_format()</i> and must be open 
 * for reading and writing. The segment file configuration parameters 
 * <i>nrows, ncols, srows, scols</i>, and <i>len</i>, as written to the 
 * file by <i>Segment_format()</i>, are read from the file and stored in 
 * the <b>seg</b> structure. <b>nsegs</b> specifies the number of 
 * segments that will be retained in memory. The minimum value allowed 
 * is 1.
 *
 * <b>Note:</b> The size of a segment is <em>scols*srows*len</em> plus a 
 * few bytes for managing each segment.
 *
 * \param[in,out] SEG segment
 * \param[in] fd file descriptor
 * \param[in] nseg number of segments to remain in memory
 * \return 1 if successful
 * \return -1 if unable to seek or read segment file
 * \return -2 if out of memory
 */

int Segment_init(SEGMENT *SEG, int fd, int nseg)
{
    SEG->open = 0;
    SEG->fd = fd;
    SEG->nseg = nseg;

    if (lseek(fd, 0L, SEEK_SET) < 0) {
	int err = errno;

	G_warning("Segment_init: %s", strerror(err));
	return -1;
    }

    /* read the header */
    if (!read_off_t(fd, &SEG->nrows)
	|| !read_off_t(fd, &SEG->ncols)
	|| !read_int(fd, &SEG->srows)
	|| !read_int(fd, &SEG->scols)
	|| !read_int(fd, &SEG->len))
	return -1;

    return seg_setup(SEG);
}


static int read_int(int fd, int *n)
{
    int bytes_read;

    if ((bytes_read = read(fd, n, sizeof(int))) == -1)
	G_warning("read_int: %s", strerror(errno));

    bytes_read = (bytes_read == sizeof(int));

    return bytes_read;
}

static int read_off_t(int fd, off_t *n)
{
    int bytes_read;

    if ((bytes_read = read(fd, n, sizeof(off_t))) == -1)
	G_warning("read_off_t: %s", strerror(errno));

    bytes_read = (bytes_read == sizeof(off_t));

    return bytes_read;
}
