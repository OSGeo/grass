#include <stdlib.h>
#include <grass/gis.h>
#include "sw_defs.h"

int ntry, totalsearch;

int ELinitialize(void)
{
    int i;

    freeinit(&hfl, sizeof(struct Halfedge));
    ELhashsize = 2 * sqrt_nsites;
    ELhash = (struct Halfedge **)G_malloc(ELhashsize * sizeof(struct Halfedge *));
    for (i = 0; i < ELhashsize; i++)
	ELhash[i] = (struct Halfedge *)NULL;
    ELleftend = HEcreate((struct Edge *)NULL, 0);
    ELrightend = HEcreate((struct Edge *)NULL, 0);
    ELleftend->ELleft = (struct Halfedge *)NULL;
    ELleftend->ELright = ELrightend;
    ELrightend->ELleft = ELleftend;
    ELrightend->ELright = (struct Halfedge *)NULL;
    ELhash[0] = ELleftend;
    ELhash[ELhashsize - 1] = ELrightend;

    return 0;
}


struct Halfedge *HEcreate(struct Edge *e, int pm)
{
    struct Halfedge *answer;

    answer = (struct Halfedge *)getfree(&hfl);
    answer->ELedge = e;
    answer->ELpm = pm;
    answer->PQnext = (struct Halfedge *)NULL;
    answer->vertex = (struct Site *)NULL;
    answer->ELrefcnt = 0;
    return (answer);
}


int ELinsert(struct Halfedge *lb, struct Halfedge *new)
{
    new->ELleft = lb;
    new->ELright = lb->ELright;
    (lb->ELright)->ELleft = new;
    lb->ELright = new;

    return 0;
}

/* Get entry from hash table, pruning any deleted nodes */
struct Halfedge *ELgethash(int b)
{
    struct Halfedge *he;

    if (b < 0 || b >= ELhashsize)
	return ((struct Halfedge *)NULL);
    he = ELhash[b];
    if (he == (struct Halfedge *)NULL || he->ELedge != (struct Edge *)DELETED)
	return (he);

    /* Hash table points to deleted half edge.  Patch as necessary. */
    ELhash[b] = (struct Halfedge *)NULL;
    if (--(he->ELrefcnt) == 0)
	makefree((struct Freenode *)he, &hfl);
    return ((struct Halfedge *)NULL);
}

struct Halfedge *ELleftbnd(struct Point *p)
{
    int i, bucket;
    struct Halfedge *he;

    /* Use hash table to get close to desired halfedge */
    bucket = (p->x - xmin) / deltax * ELhashsize;
    if (bucket < 0)
	bucket = 0;
    if (bucket >= ELhashsize)
	bucket = ELhashsize - 1;
    he = ELgethash(bucket);
    if (he == (struct Halfedge *)NULL) {
	for (i = 1; 1; i++) {
	    if ((he = ELgethash(bucket - i)) != (struct Halfedge *)NULL)
		break;
	    if ((he = ELgethash(bucket + i)) != (struct Halfedge *)NULL)
		break;
	}
	totalsearch += i;
    }
    ntry++;
    /* Now search linear list of halfedges for the corect one */
    if (he == ELleftend || (he != ELrightend && right_of(he, p))) {
	do {
	    he = he->ELright;
	} while (he != ELrightend && right_of(he, p));
	he = he->ELleft;
    }
    else
	do {
	    he = he->ELleft;
	} while (he != ELleftend && !right_of(he, p));

    /* Update hash table and reference counts */
    if (bucket > 0 && bucket < ELhashsize - 1) {
	if (ELhash[bucket] != (struct Halfedge *)NULL)
	    ELhash[bucket]->ELrefcnt--;
	ELhash[bucket] = he;
	ELhash[bucket]->ELrefcnt++;
    }
    return (he);
}


/* This delete routine can't reclaim node, since pointers from hash
   table may be present.   */
int ELdelete(struct Halfedge *he)
{
    (he->ELleft)->ELright = he->ELright;
    (he->ELright)->ELleft = he->ELleft;
    he->ELedge = (struct Edge *)DELETED;

    return 0;
}


struct Halfedge *ELright(struct Halfedge *he)
{
    return (he->ELright);
}

struct Halfedge *ELleft(struct Halfedge *he)
{
    return (he->ELleft);
}


struct Site *leftreg(struct Halfedge *he)
{
    if (he->ELedge == (struct Edge *)NULL)
	return (bottomsite);
    return (he->ELpm == le ? he->ELedge->reg[le] : he->ELedge->reg[re]);
}

struct Site *rightreg(struct Halfedge *he)
{
    if (he->ELedge == (struct Edge *)NULL)
	return (bottomsite);
    return (he->ELpm == le ? he->ELedge->reg[re] : he->ELedge->reg[le]);
}
