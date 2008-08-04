#include <stdio.h>
#include <grass/btree.h>

int btree_find(const BTREE * B, const void *key, void **data)
{
    int q;
    int dir;


    if (B->N <= 0)
	return 0;

    q = 1;
    while (q > 0) {
	dir = (*B->cmp) (B->node[q].key, key);
	if (dir == 0) {
	    *data = B->node[q].data;
	    return 1;
	}
	if (dir > 0)
	    q = B->node[q].left;	/* go left */
	else
	    q = B->node[q].right;	/* go right */
    }

    return 0;
}
