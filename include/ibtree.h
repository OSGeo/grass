#ifndef GRASS_IBTREE_H
#define GRASS_IBTREE_H

typedef struct
{
    int key;
    int data;
    int left;
    int right;
} IBTREE_NODE;

typedef struct
{
    IBTREE_NODE *node;		/* tree of values */
    int tlen;			/* allocated tree size */
    int N;			/* number of actual nodes in tree */
    int incr;			/* number of nodes to add at a time */
    int cur;
    int (*cmp) ();		/* routine to compare keys */
} IBTREE;

#include <grass/defs/ibtree.h>

#endif
