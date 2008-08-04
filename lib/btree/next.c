#include <grass/btree.h>

int btree_next(BTREE * B, void **key, void **data)
{
    int q;

    if (B->N <= 0)
	return 0;

    /* if rewound, start at root and go all the way to the left */
    if (B->cur == 0)
	B->cur = 1;

    /* go to the right */
    else
	B->cur = B->node[B->cur].right;

    if (B->cur == 0)		/* no more */
	return 0;

    if (B->cur < 0)		/* thread. stop here */
	B->cur = -(B->cur);
    else			/* go all the way left */
	while ((q = B->node[B->cur].left))
	    B->cur = q;

    *key = B->node[B->cur].key;
    *data = B->node[B->cur].data;

    return 1;
}
