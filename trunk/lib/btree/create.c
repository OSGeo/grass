#include <stdio.h>
#include <stdlib.h>
#include <grass/btree.h>

int btree_create(BTREE * B, int (*cmp) (const void *, const void *), int incr)
{
    if (incr <= 0)
	incr = 1;

    B->N = 0;
    B->cur = 0;
    B->tlen = B->incr = incr;

    /* must have at least 2 nodes, since node[0] is never used */
    if (B->tlen == 1)
	B->tlen = 2;

    B->cmp = cmp;
    B->node = (BTREE_NODE *) malloc(B->tlen * sizeof(BTREE_NODE));
    if (B->node == NULL)
	return 0;
    return 1;
}
