
/**
 * \file seek.c
 *
 * \brief Segment seek routines.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2005-2006
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/segment.h>


/**
 * \fn int segment_seek (SEGMENT *SEG, int n, int index)
 *
 * \brief Seek into a segment.
 *
 * \param[in,out] SEG segment
 * \param[in] n
 * \param[in] index
 * \return 0 on success
 * \return -1 if unable to seek
 */

int segment_seek(const SEGMENT * SEG, int n, int index)
{
    off_t offset;

    offset = (off_t) n *SEG->size + index + SEG->offset;

    if (lseek(SEG->fd, offset, SEEK_SET) == (off_t) - 1) {
	G_warning("segment_seek: %s", strerror(errno));
	return -1;
    }

    return 0;
}
