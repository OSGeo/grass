
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
 * \date 2005-2006
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <grass/segment.h>


static int segment_select(SEGMENT *, int);


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
    int age;
    int cur;
    int i;
    int read_result;

    /* is n the current segment? */
    if (n == SEG->scb[SEG->cur].n)
	return SEG->cur;

    /* search the in memory segments */
    for (i = 0; i < SEG->nseg; i++)
	if (n == SEG->scb[i].n)
	    return segment_select(SEG, i);

    /* find a slot to use to hold segment */
    age = 0;
    cur = 0;
    for (i = 0; i < SEG->nseg; i++)
	if (SEG->scb[i].n < 0) {	/* free slot */
	    cur = i;
	    break;
	}
	else if (age < SEG->scb[i].age) {	/* find oldest segment */
	    cur = i;
	    age = SEG->scb[i].age;
	}

    /* if slot is used, write it out, if dirty */
    if (SEG->scb[cur].n >= 0 && SEG->scb[cur].dirty)
	if (segment_pageout(SEG, cur) < 0)
	    return -1;

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

    return segment_select(SEG, cur);
}


static int segment_select(SEGMENT * SEG, int n)
{
    int i;

    SEG->scb[n].age = 0;
    for (i = 0; i < SEG->nseg; i++)
	SEG->scb[i].age++;

    return SEG->cur = n;
}
