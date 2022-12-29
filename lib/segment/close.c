/**
 * \file lib/segment/close.c
 *
 * \brief Segment closing routine.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2018
 */

#include <unistd.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

/**
 * \fn int Segment_close (SEGMENT *SEG)
 *
 * \brief Free memory allocated to segment, delete temp file.
 *
 * Releases the allocated memory associated with the segment file 
 * <b>seg</b> and deletes the temporary file.
 *
 * \param[in,out] SEG segment
 * \return 1 if successful
 * \return -1 if SEGMENT is not available (not open)
 */

int Segment_close(SEGMENT *SEG)
{
    if (SEG->open != 1)
	return -1;

    if (SEG->cache) {
	G_free(SEG->cache);
    }
    else {
	Segment_release(SEG);
	close(SEG->fd);
	unlink(SEG->fname);

	SEG->fd = -1;
	SEG->fname = NULL;
    }

    SEG->open = 0;

    return 1;
}
