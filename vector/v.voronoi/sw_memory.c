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
	t = (struct Freenode *)myalloc(sqrt_nsites * fl->nodesize);
	for (i = 0; i < sqrt_nsites; i += 1)
	    makefree((struct Freenode *)((char *)t + i * fl->nodesize), fl);
    };
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

int total_alloc;
char *myalloc(unsigned n)
{
    char *t;

    if ((t = G_malloc(n)) == (char *)0) {
	fprintf(stderr,
		"Insufficient memory processing site %d (%d bytes in use)\n",
		siteidx, total_alloc);
	exit(0);
    };
    total_alloc += n;
    return (t);
}
