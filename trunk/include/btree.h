#ifndef GRASS_BTREE_H
#define GRASS_BTREE_H

typedef struct
{
    void *key;
    void *data;
    int left;
    int right;
} BTREE_NODE;

typedef struct
{
    BTREE_NODE *node;		/* tree of values */
    int tlen;			/* allocated tree size */
    int N;			/* number of actual nodes in tree */
    int incr;			/* number of nodes to add at a time */
    int cur;
    int (*cmp) (const void *, const void *);	/* routine to compare keys */
} BTREE;

#include <grass/defs/btree.h>

#endif
