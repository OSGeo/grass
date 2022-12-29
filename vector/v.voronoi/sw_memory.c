#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include "sw_defs.h"

int freeinit(struct Freelist *fl, int size)
{
    fl->head = (struct Freenode *)NULL;
    fl->nodesize = size;
    return 0;
}

char *getfree(struct Freelist *fl)
{
    int i;
    struct Freenode *t;

    if (fl->head == (struct Freenode *)NULL) {
	t = (struct Freenode *)G_malloc(sqrt_nsites * fl->nodesize);
	for (i = 0; i < sqrt_nsites; i++)
	    makefree((struct Freenode *)((char *)t + i * fl->nodesize), fl);
    }
    t = fl->head;
    fl->head = (fl->head)->nextfree;
    return ((char *)t);
}

int makefree(struct Freenode *curr, struct Freelist *fl)
{
    curr->nextfree = fl->head;
    fl->head = curr;
    return 0;
}
