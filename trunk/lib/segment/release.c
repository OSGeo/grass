
/**
 * \file lib/segment/release.c
 *
 * \brief Segment release routines.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2005-2009
 */

#include <stdlib.h>
#include <grass/gis.h>
#include "local_proto.h"


/**
 * \fn int Segment_release (SEGMENT *SEG)
 *
 * \brief Free memory allocated to segment.
 *
 * Releases the allocated memory associated with the segment file 
 * <b>seg</b>.
 *
 * <b>Note:</b> Does not close the file. Does not flush the data which 
 * may be pending from previous <i>Segment_put()</i> calls.
 *
 * \param[in,out] SEG segment
 * \return 1 if successful
 * \return -1 if SEGMENT is not available (not open)
 */

int Segment_release(SEGMENT * SEG)
{
    int i;

    if (SEG->open != 1)
	return -1;

    for (i = 0; i < SEG->nseg; i++)
	G_free(SEG->scb[i].buf);
    G_free(SEG->scb);

    G_free(SEG->freeslot);
    G_free(SEG->agequeue);
    G_free(SEG->load_idx);

    SEG->open = 0;

    return 1;
}
