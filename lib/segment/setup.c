
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
 * \date 2005-2009
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/segment.h>

#include "rbtree.h"


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
	G_warning("segment_setup: illegal segment file parameters");
	return -1;
    }

    /* This is close to the beginning of the file, so doesn't need to be an off_t */
    SEG->offset = (int)lseek(SEG->fd, 0L, SEEK_CUR);

    SEG->spr = SEG->ncols / SEG->scols;
    SEG->spill = SEG->ncols % SEG->scols;
    if (SEG->spill)
	SEG->spr++;

    /* fast address */
    SEG->slow_adrs = 1;
    if (SEG->scols - pow(2, (log(SEG->scols) / log(2))) == 0) {
	if (SEG->srows - pow(2, (log(SEG->srows) / log(2))) == 0) {
	    SEG->scolbits = log(SEG->scols) / log(2);
	    SEG->srowbits = log(SEG->srows) / log(2);
	    SEG->segbits = SEG->srowbits + SEG->scolbits;
	    SEG->slow_adrs = 0;
	    G_debug(1, "segment lib: fast address activated");
	}
    }
    /* fast seek */
    SEG->slow_seek = 1;
    if (SEG->slow_adrs == 0) {
	if (SEG->len - pow(2, (log(SEG->len) / log(2))) == 0) {
	    SEG->lenbits = log(SEG->len) / log(2);
	    SEG->sizebits = SEG->segbits + SEG->lenbits;
	    SEG->slow_seek = 0;
	    G_debug(1, "segment lib: fast seek activated");
	}
    }

    /* adjust number of open segments if larger than number of total segments */
    if (SEG->nseg > SEG->spr * ((SEG->nrows + SEG->srows - 1) / SEG->srows)) {
	G_warning("segment: reducing number of open segments from %d to %d",
		   SEG->nseg, SEG->spr * ((SEG->nrows + SEG->srows - 1) / SEG->srows));
	SEG->nseg = SEG->spr * ((SEG->nrows + SEG->srows - 1) / SEG->srows);
    }

    if ((SEG->scb =
	 (struct SEGMENT_SCB *)G_malloc(SEG->nseg *
					sizeof(struct SEGMENT_SCB))) == NULL)
	return -2;

    if ((SEG->freeslot = (int *)G_malloc(SEG->nseg * sizeof(int))) == NULL)
	return -2;

    if ((SEG->agequeue = (struct aq *)G_malloc((SEG->nseg + 1) * sizeof(struct aq))) == NULL)
	return -2;

    SEG->srowscols = SEG->srows * SEG->scols;
    SEG->size = SEG->srowscols * SEG->len;
    
    for (i = 0; i < SEG->nseg; i++) {
	if ((SEG->scb[i].buf = G_malloc(SEG->size)) == NULL)
	    return -2;

	SEG->scb[i].n = -1;	/* mark free */
	SEG->scb[i].dirty = 0;
	SEG->scb[i].age = NULL;
	SEG->freeslot[i] = i;
	SEG->agequeue[i].cur = -1;
	if (i > 0) {
	    SEG->agequeue[i].younger = &(SEG->agequeue[i - 1]);
	    SEG->agequeue[i].older = &(SEG->agequeue[i + 1]);
	}
	else if (i == 0) {
	    SEG->agequeue[i].younger = &(SEG->agequeue[SEG->nseg]);
	    SEG->agequeue[i].older = &(SEG->agequeue[i + 1]);
	}
    }
    
    SEG->agequeue[SEG->nseg].cur = -1;
    SEG->agequeue[SEG->nseg].younger = &(SEG->agequeue[SEG->nseg - 1]);
    SEG->agequeue[SEG->nseg].older = &(SEG->agequeue[0]);
    SEG->youngest = SEG->oldest = &(SEG->agequeue[SEG->nseg]);
    
    SEG->nfreeslots = SEG->nseg;
    SEG->cur = 0;
    SEG->open = 1;
    
    SEG->loaded = rbtree_create(segment_compare, sizeof(SEGID));

    return 1;
}

int segment_compare(const void *sega, const void *segb)
{
    SEGID *a, *b;
    
    a = (SEGID *)sega;
    b = (SEGID *)segb;
    
    if (a->n > b->n)
	return 1;
    else if (a->n < b->n)
	return -1;
    
    return 0;
}
