
/**
 * \file pagein.c
 *
 * \brief Segment page-in routines.
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
#include <grass/segment.h>
#include "rbtree.h"


/**
 * \fn int segment_pagein (SEGMENT *SEG, int n)
 *
 * \brief Segment pagein.
 *
 * Finds <b>n</b> in the segment file, <b>seg</b>, and selects it as the 
 * current segment.
 *
 * \param[in] seg segment
 * \param[in] n segment number
 * \return 1 if successful
 * \return -1 if unable to seek or read segment file
 */

int segment_pagein(SEGMENT * SEG, int n)
{
    int cur;
    int read_result;
    SEGID *seg_found, seg_search;

    /* is n the current segment? */
    if (n == SEG->scb[SEG->cur].n)
	return SEG->cur;
	
    /* search the in memory segments */
    seg_search.i = 0;
    seg_search.n = n;
    seg_found = rbtree_find(SEG->loaded, &seg_search);
    if (seg_found) {
	cur = seg_found->i;

	if (SEG->scb[cur].age != SEG->youngest) {
	    /* splice out */
	    SEG->scb[cur].age->younger->older = SEG->scb[cur].age->older;
	    SEG->scb[cur].age->older->younger = SEG->scb[cur].age->younger;
	    /* splice in */
	    SEG->scb[cur].age->younger = SEG->youngest->younger;
	    SEG->scb[cur].age->older = SEG->youngest;
	    SEG->scb[cur].age->older->younger = SEG->scb[cur].age;
	    SEG->scb[cur].age->younger->older = SEG->scb[cur].age;
	    /* make it youngest */
	    SEG->youngest = SEG->scb[cur].age;
	}
	
	return SEG->cur = cur;
    }
    
    /* find a slot to use to hold segment */
    if (SEG->nfreeslots) {  /* any free slots left ? */
	cur = SEG->freeslot[--SEG->nfreeslots];
    }
    else {	/* find oldest segment */
	SEG->oldest = SEG->oldest->younger;
	cur = SEG->oldest->cur;
	SEG->oldest->cur = -1;
	SEG->scb[cur].age = NULL;
    }

    /* if slot is used, write it out, if dirty */
    if (SEG->scb[cur].n >= 0 && SEG->scb[cur].dirty) {
	if (segment_pageout(SEG, cur) < 0)
	    return -1;
    }
	
    if (SEG->scb[cur].n >= 0) {
	seg_search.n = SEG->scb[cur].n;
	if (rbtree_remove(SEG->loaded, &seg_search) == 0)
	    G_fatal_error("could not remove segment");
	seg_search.n = n;
    }

    /* read in the segment */
    SEG->scb[cur].n = n;
    SEG->scb[cur].dirty = 0;
    segment_seek(SEG, SEG->scb[cur].n, 0);

    read_result = read(SEG->fd, SEG->scb[cur].buf, SEG->size);
    if (read_result != SEG->size) {
	G_debug(2, "segment_pagein: read_result=%d  SEG->size=%d",
		read_result, SEG->size);

	if (read_result < 0)
	    G_warning("segment_pagein: %s", strerror(errno));
	else if (read_result == 0)
	    G_warning("segment_pagein: read EOF");
	else
	    G_warning
		("segment_pagein: short count during read(), got %d, expected %d",
		 read_result, SEG->size);

	return -1;
    }

    if (cur < 0 || n < 0)
	G_fatal_error("segment not loaded");

    /* remember loaded segment */
    seg_search.i = cur;
    if (rbtree_insert(SEG->loaded, &seg_search) == 0)
	G_fatal_error("could not insert segment");

    /* make it youngest segment */
    SEG->youngest = SEG->youngest->younger;
    SEG->scb[cur].age = SEG->youngest;
    SEG->youngest->cur = cur;
    
    return SEG->cur = cur;
}
