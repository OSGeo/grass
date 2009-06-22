/* -*-c-basic-offset: 4;-*-
 *  Chained memory allocator 
 *  memory.c
 *
 *  Pierre de Mouveaux (pmx)  
 *  pmx@audiovu.com  - 10 april 2000.  
 *
 *  Used in GRASS 5.0 r.cost module
 *
 *  Released under GPL
 */



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

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int allocate(void)
{
    struct cost *data, *p1, *p2;
    int i;

    G_debug(2, "allocate()\n");

    data = (struct cost *)G_malloc(NUM_IN_BLOCK * sizeof(struct cost));

    if (data == NULL) {
	G_warning("allocat(): error %s\n", strerror(errno));
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

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int release(void)
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

    return 1;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
struct cost *get(void)
{
    struct cost *p;

    if (first_free == NULL) {
	allocate();
    }

    p = first_free;
    first_free = p->lower;
    if (first_free->lower == NULL) {
	allocate();
    }
    return p;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
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
