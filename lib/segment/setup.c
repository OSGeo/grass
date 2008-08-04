
/**
 * \file setup.c
 *
 * \brief Segment setup routines.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2005-2006
 */

#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/segment.h>


/**
 * \fn int segment_setup (SEGMENT *SEG)
 *
 * \brief Setup segment.
 *
 * <b>SEG</b> must have the following parms set:
 *  fd (open for read and write), nrows, ncols, srows, scols, len, nseg
 *
 * \param[in,out] SEG segment
 * \return 1 if successful
 * \return -1 if illegal parameters are passed in <b>SEG</b>
 * \return -2 if unable to allocate memory
 */

int segment_setup(SEGMENT * SEG)
{
    int i;

    SEG->open = 0;

    if (SEG->nrows <= 0 || SEG->ncols <= 0
	|| SEG->srows <= 0 || SEG->scols <= 0
	|| SEG->len <= 0 || SEG->nseg <= 0) {
	G_warning("segment_setup: illegal segment file parameters\n");
	return -1;
    }

    /* This is close to the beginning of the file, so doesn't need to be an off_t */
    SEG->offset = (int)lseek(SEG->fd, 0L, SEEK_CUR);

    SEG->spr = SEG->ncols / SEG->scols;
    SEG->spill = SEG->ncols % SEG->scols;
    if (SEG->spill)
	SEG->spr++;

    if ((SEG->scb =
	 (struct SEGMENT_SCB *)G_malloc(SEG->nseg *
					sizeof(struct SEGMENT_SCB))) == NULL)
	return -2;

    SEG->size = SEG->srows * SEG->scols * SEG->len;

    for (i = 0; i < SEG->nseg; i++) {
	if ((SEG->scb[i].buf = G_malloc(SEG->size)) == NULL)
	    return -2;

	SEG->scb[i].n = -1;	/* mark free */
	SEG->scb[i].dirty = 0;
	SEG->scb[i].age = 0;
    }
    SEG->cur = 0;
    SEG->open = 1;

    return 1;
}
