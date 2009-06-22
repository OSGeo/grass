
/****************************************************************************
 *
 * MODULE:       r.cost
 *
 * AUTHOR(S):    Antony Awaida - IESL - M.I.T.
 *               James Westervelt - CERL
 *               Pierre de Mouveaux <pmx audiovu com>
 *               Eric G. Miller <egm2 jps net>
 *
 * PURPOSE:      Outputs a raster map layer showing the cumulative cost
 *               of moving between different geographic locations on an
 *               input raster map layer whose cell category values
 *               represent cost.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <grass/gis.h>
#include "memory.h"

#define NUM_IN_BLOCK	1024*8


struct cost *first_free = NULL;
struct cost *first = NULL;
struct cost *last = NULL;


int allocate()
{
    struct cost *data, *p1, *p2;
    int i;

    /*      fprintf(stderr,"allocate()\n"); */

    data = (struct cost *)G_malloc(NUM_IN_BLOCK * sizeof(struct cost));

    if (data == NULL) {
	/* G_warning( */
	fprintf(stderr, "allocat(): error %s\n", strerror(errno));
	return 0;
    }

    if (last != NULL) {
	last->lower = data;
	data->higher = last;
    }


    p1 = p2 = data;
    p2++;

    for (i = 1; i < NUM_IN_BLOCK - 1; i++, p1++, p2++) {
	p1->lower = p2;
	p2->higher = p1;
	p1->above = NULL;
    }
    p2->higher = p1;
    p2->above = NULL;
    p2->lower = NULL;
    last = p2;

    if (first == NULL) {
	first_free = data;
	first = data;
    }
    else {
	first_free->lower = data;
    }

    return 1;
}

int release()
{
    struct cost *p = first;
    struct cost *next;

    if (p == NULL)
	return 1;

    do {
	next = (p + NUM_IN_BLOCK)->lower;
	G_free(p);
	p = next;
    } while (next != NULL);

    first = last = first_free = NULL;

    return 0;
}

struct cost *get()
{
    struct cost *p;

    if (first_free == NULL) {
	if (allocate() < 0) {
	    /* exit(1); */
	}
    }

    p = first_free;
    first_free = p->lower;
    if (first_free->lower == NULL) {
	if (allocate() < 0) {
	    /* exit(1); */
	}
    }
    return p;
}

int give(struct cost *p)
{
    if (p == NULL)
	return 0;

    p->lower = first_free;
    first_free->above = p;
    first_free = p;
    p->above = NULL;		/* not used in this chain - (pmx) */

    return 1;
}
