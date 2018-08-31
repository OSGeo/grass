#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/btree.h>

static void *store(const void *, int);

int btree_update(BTREE * B,
		 const void *key, int keylen, const void *data, int datalen)
{
    int p = 0;
    int q;
    int N;
    int (*cmp) ();
    int dir;

    /* first node is special case */
    N = B->N;
    if (N == 0) {
	N = B->N = 1;
	B->node[N].key = store(key, keylen);
	B->node[N].data = store(data, datalen);
	B->node[N].left = 0;
	B->node[N].right = 0;
	if (B->node[N].key && B->node[N].data)
	    return 1;
	else
	    return 0;
    }

    cmp = B->cmp;

    q = 1;
    while (q > 0) {
	p = q;
	dir = (*cmp) (B->node[q].key, key);
	if (dir == 0) {
	    free(B->node[q].data);
	    return ((B->node[q].data = store(data, datalen))) ? 1 : 0;
	}
	if (dir > 0)
	    q = B->node[q].left;	/* go left */
	else
	    q = B->node[q].right;	/* go right */
    }

    /* new node */
    B->N++;
    N = B->N;

    /* grow the tree? */
    if (N >= B->tlen) {
	B->node =
	    (BTREE_NODE *) realloc(B->node,
				   sizeof(BTREE_NODE) * (B->tlen += B->incr));
	if (B->node == NULL)
	    return 0;
    }

    /* add node to tree */
    B->node[N].key = store(key, keylen);
    B->node[N].data = store(data, datalen);
    B->node[N].left = 0;

    if (dir > 0) {
	B->node[N].right = -p;	/* create thread */
	B->node[p].left = N;	/* insert left */
    }
    else {
	B->node[N].right = B->node[p].right;	/* copy right link/thread */
	B->node[p].right = N;	/* add right */
    }
    return 1;
}

static void *store(const void *s, int n)
{
    void *b;

    if (n <= 0)
	return NULL;

    b = malloc(n);
    if (b == NULL)
	return b;

    memcpy(b, s, n);

    return b;
}
