
/**
 * \file lib/segment/setup.c
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
#include "local_proto.h"


/**
 * \brief Internal use only
 * 
 * Setup segment.
 *
 * <b>SEG</b> must have the following parms set:
 *  fd (open for read and write), nrows, ncols, srows, scols, len, nseg
 *
 * \param[in,out] SEG segment
 * \return 1 if successful
 * \return -1 if illegal parameters are passed in <b>SEG</b>
 * \return -2 if unable to allocate memory
 */

int seg_setup(SEGMENT * SEG)
{
    int i;
    int seg_exp, n_total_segs;

    SEG->open = 0;
    SEG->cache = NULL;

    if (SEG->nrows <= 0 || SEG->ncols <= 0
	|| SEG->srows <= 0 || SEG->scols <= 0
	|| SEG->len <= 0 || SEG->nseg <= 0) {
	G_warning("Segment setup: illegal segment file parameters");
	return -1;
    }

    /* This is close to the beginning of the file, so doesn't need to be an off_t */
    SEG->offset = (int)lseek(SEG->fd, 0L, SEEK_CUR);

    SEG->spr = SEG->ncols / SEG->scols;
    SEG->spill = SEG->ncols % SEG->scols;
    if (SEG->spill)
	SEG->spr++;

    /* fast address */
    SEG->fast_adrs = 0;

    seg_exp = 0;
    while (SEG->scols - (1 << seg_exp) > 0)
	seg_exp++;

    if (SEG->scols - (1 << seg_exp) == 0) {
	SEG->scolbits = seg_exp;
	seg_exp = 0;
	while (SEG->srows - (1 << seg_exp) > 0)
	    seg_exp++;
	if (SEG->srows - (1 << seg_exp) == 0) {
	    SEG->srowbits = seg_exp;
	    SEG->segbits = SEG->srowbits + SEG->scolbits;
	    SEG->fast_adrs = 1;
	    G_debug(1, "Segment setup: fast address activated");
	}
    }
    if (SEG->fast_adrs)
	SEG->address = seg_address_fast;
    else
	SEG->address = seg_address_slow;
    
    /* fast seek */
    SEG->fast_seek = 0;
    if (SEG->fast_adrs == 1) {
	seg_exp = 0;
	while (SEG->len - (1 << seg_exp) > 0)
	    seg_exp++;
	if (SEG->len - (1 << seg_exp) == 0) {
	    SEG->lenbits = seg_exp;
	    SEG->sizebits = SEG->segbits + SEG->lenbits;
	    SEG->fast_seek = 1;
	    G_debug(1, "Segment setup: fast seek activated");
	}
    }
    if (SEG->fast_seek)
	SEG->seek = seg_seek_fast;
    else
	SEG->seek = seg_seek_slow;

    /* adjust number of open segments if larger than number of total segments */
    n_total_segs = SEG->spr * ((SEG->nrows + SEG->srows - 1) / SEG->srows);
    if (SEG->nseg > n_total_segs) {
	G_debug(1, "Segment setup: reducing number of open segments from %d to %d",
		  SEG->nseg, n_total_segs);
	SEG->nseg = n_total_segs;
    }

    if ((SEG->scb =
	 (struct scb *)G_malloc(SEG->nseg *
					sizeof(struct scb))) == NULL)
	return -2;

    if ((SEG->freeslot = (int *)G_malloc(SEG->nseg * sizeof(int))) == NULL)
	return -2;

    if ((SEG->agequeue =
	 (struct aq *)G_malloc((SEG->nseg + 1) * sizeof(struct aq))) == NULL)
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

    /* index for each segment, same like cache of r.proj */

    /* alternative using less memory: RB Tree */
    /* SEG->loaded = rbtree_create(cmp, sizeof(SEGID)); */
    /* SEG->loaded = NULL; */

    SEG->load_idx = G_malloc(n_total_segs * sizeof(int));

    for (i = 0; i < n_total_segs; i++)
	SEG->load_idx[i] = -1;

    return 1;
}
