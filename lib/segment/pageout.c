
/**
 * \file pageout.c
 *
 * \brief Segment page-out routines.
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
#include "local_proto.h"


/**
 * \brief Internal use only
 * 
 * Pages segment to disk.
 *
 * Finds segment value <b>i</b> in segment <b>seg</b> and pages it out 
 * to disk.
 *
 * \param[in] SEG segment
 * \param[in] i segment value
 * \return 1 if successful
 * \return -1 on error
 */

int seg_pageout(SEGMENT * SEG, int i)
{
    SEG->seek(SEG, SEG->scb[i].n, 0);
    errno = 0;
    if (write(SEG->fd, SEG->scb[i].buf, SEG->size) != SEG->size) {
	int err = errno;

	if (err)
	    G_warning("Segment pageout: %s", strerror(err));
	else
	    G_warning("Segment pageout: insufficient disk space?");
	return -1;
    }
    SEG->scb[i].dirty = 0;

    return 1;
}
