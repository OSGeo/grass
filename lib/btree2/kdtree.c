/*!
 * \file kdtree.c
 *
 * \brief binary search tree 
 *
 * Dynamic balanced k-d tree implementation
 *
 * (C) 2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2).  Read the file COPYING that comes with GRASS for details.
 *
 * \author Markus Metz
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "kdtree.h"

#ifdef MAX
#undef MAX
#endif
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define KD_BTOL 7

#ifdef KD_DEBUG
#undef KD_DEBUG
#endif

static int rcalls = 0;
static int rcallsmax = 0;

static struct kdnode *kdtree_insert2(struct kdtree *, struct kdnode *,
                                     struct kdnode *, int, int);
static int kdtree_replace(struct kdtree *, struct kdnode *);
static int kdtree_balance(struct kdtree *, struct kdnode *, int);
static int kdtree_first(struct kdtrav *, double *, int *);
static int kdtree_next(struct kdtrav *, double *, int *);

static int cmp(struct kdnode *a, struct kdnode *b, int p)
{
    if (a->c[p] < b->c[p])
	return -1;
    if (a->c[p] > b->c[p])
	return 1;

    return (a->uid < b->uid ? -1 : a->uid > b->uid);
}

static int cmpc(struct kdnode *a, struct kdnode *b, struct kdtree *t)
{
    int i;
    for (i = 0; i < t->ndims; i++) {
	if (a->c[i] != b->c[i]) {
	    return 1;
	}
    }

    return 0;
}

static struct kdnode *kdtree_newnode(struct kdtree *t)
{
    struct kdnode *n = G_malloc(sizeof(struct kdnode));
    
    n->c = G_malloc(t->ndims * sizeof(double));
    n->dim = 0;
    n->depth = 0;
    n->balance = 0;
    n->uid = 0;
    n->child[0] = NULL;
    n->child[1] = NULL;
    
    return n;
}

static void kdtree_free_node(struct kdnode *n)
{
    G_free(n->c);
    G_free(n);
}

static void kdtree_update_node(struct kdtree *t, struct kdnode *n)
{
    int ld, rd, btol;

    ld = (!n->child[0] ? -1 : n->child[0]->depth);
    rd = (!n->child[1] ? -1 : n->child[1]->depth);
    n->depth = MAX(ld, rd) + 1;

    n->balance = 0;
    /* set balance flag if any of the node's subtrees needs balancing
     * or if the node itself needs balancing */
    if ((n->child[0] && n->child[0]->balance) ||
        (n->child[1] && n->child[1]->balance)) {
	n->balance = 1;

	return;
    }

    btol = t->btol;
    if (!n->child[0] || !n->child[1])
	btol = 2;

    if (ld > rd + btol || rd > ld + btol) 
	n->balance = 1;
}

/* create a new k-d tree with ndims dimensions,
 * optionally set balancing tolerance */
struct kdtree *kdtree_create(char ndims, int *btol)
{
    int i;
    struct kdtree *t;
    
    t = G_malloc(sizeof(struct kdtree));
    
    t->ndims = ndims;
    t->csize = ndims * sizeof(double);
    t->btol = KD_BTOL;
    if (btol) {
	t->btol = *btol;
	if (t->btol < 2)
	    t->btol = 2;
    }

    t->nextdim = G_malloc(ndims * sizeof(char));
    for (i = 0; i < ndims - 1; i++)
	t->nextdim[i] = i + 1;
    t->nextdim[ndims - 1] = 0;

    t->count = 0;
    t->root = NULL;

    return t;
}

/* clear the tree, removing all entries */
void kdtree_clear(struct kdtree *t)
{
    struct kdnode *it;
    struct kdnode *save = t->root;

    /*
    Rotate away the left links so that
    we can treat this like the destruction
    of a linked list
    */
    while((it = save) != NULL) {
	if (it->child[0] == NULL) {
	    /* No left links, just kill the node and move on */
	    save = it->child[1];
	    kdtree_free_node(it);
	    it = NULL;
	}
	else {
	    /* Rotate away the left link and check again */
	    save = it->child[0];
	    it->child[0] = save->child[1];
	    save->child[1] = it;
	}
    }
    t->root = NULL;
}

/* destroy the tree */
void kdtree_destroy(struct kdtree *t)
{
    /* remove all entries */
    kdtree_clear(t);
    G_free(t->nextdim);

    G_free(t);
    t = NULL;
}

/* insert an item (coordinates c and uid) into the k-d tree
 * dc == 1: allow duplicate coordinates */
int kdtree_insert(struct kdtree *t, double *c, int uid, int dc)
{
    struct kdnode *nnew;
    size_t count = t->count;

    nnew = kdtree_newnode(t);
    memcpy(nnew->c, c, t->csize);
    nnew->uid = uid;

    t->root = kdtree_insert2(t, t->root, nnew, 1, dc);

    /* print depth of recursion
     * recursively called fns are insert2, balance, and replace */
    /*
    if (rcallsmax > 1)
	fprintf(stdout, "%d\n", rcallsmax);
    */

    return count < t->count;
}

/* remove an item from the k-d tree
 * coordinates c and uid must match */
int kdtree_remove(struct kdtree *t, double *c, int uid)
{
    struct kdnode sn, *n;
    struct kdstack {
	struct kdnode *n;
	int dir;
    } s[256];
    int top;
    int dir, found;
    int balance, bmode;

    sn.c = c;
    sn.uid = uid;

    /* find sn node */
    top = 0;
    s[top].n = t->root;
    dir = 1;
    found = 0;
    while (!found) {
	n = s[top].n;
	found = (!cmpc(&sn, n, t) && sn.uid == n->uid);
	if (!found) {
	    dir = cmp(&sn, n, n->dim) > 0;
	    s[top].dir = dir;
	    top++;
	    s[top].n = n->child[dir];

	    if (!s[top].n) {
		G_warning("Node does not exist");
		
		return 0;
	    }
	}
    }

    if (s[top].n->depth == 0) {
	kdtree_free_node(s[top].n);
	s[top].n = NULL;
	if (top) {
	    top--;
	    n = s[top].n;
	    dir = s[top].dir;
	    n->child[dir] = NULL;

	    /* update node */
	    kdtree_update_node(t, n);
	}
	else {
	    t->root = NULL;

	    return 1;
	}
    }
    else
	kdtree_replace(t, s[top].n);

    while (top) {
	top--;
	n = s[top].n;

	/* update node */
	kdtree_update_node(t, n);
    }

    balance = 1;
    bmode = 1;
    if (balance) {
	struct kdnode *r;
	int iter, bmode2;

	/* fix any inconsistencies in the (sub-)tree */
	iter = 0;
	bmode2 = 0;
	top = 0;
	r = t->root;
	s[top].n = r;
	while (top >= 0) {

	    n = s[top].n;

	    /* top-down balancing
	     * slower but more compact */
	    if (!bmode2) {
		while (kdtree_balance(t, n, bmode));
	    }

	    /* go down */
	    if (n->child[0] && n->child[0]->balance) {
		dir = 0;
		top++;
		s[top].n = n->child[dir];
	    }
	    else if (n->child[1] && n->child[1]->balance) {
		dir = 1;
		top++;
		s[top].n = n->child[dir];
	    }
	    /* go back up */
	    else {

		/* bottom-up balancing
		 * faster but less compact */
		kdtree_update_node(t, n);
		if (bmode2) {
		    while (kdtree_balance(t, n, bmode));
		}
		top--;
		if (top >= 0) {
		    kdtree_update_node(t, s[top].n);
		}
		if (!bmode2 && top == 0) {
		    iter++;
		    if (iter == 2) {
			/* the top node has been visited twice, 
			 * switch from top-down to bottom-up balancing */
			iter = 0;
			bmode2 = 1;
		    }
		}
	    }
	}
    }

    return 1;
}

/* k-d tree optimization, only useful if the tree will be used heavily
 * (more searches than items in the tree)
 * level 0 = a bit, 1 = more, 2 = a lot */
void kdtree_optimize(struct kdtree *t, int level)
{
    struct kdnode *n, *n2;
    struct kdstack {
	struct kdnode *n;
	int dir;
	char v;
    } s[256];
    int dir;
    int top;
    int ld, rd;
    int diffl, diffr;
    int nbal;

    if (!t->root)
	return;

    G_debug(1, "k-d tree optimization for %zd items, tree depth %d",
            t->count, t->root->depth);

    nbal = 0;
    top = 0;
    s[top].n = t->root;
    while (s[top].n) {
	n = s[top].n;

	ld = (!n->child[0] ? -1 : n->child[0]->depth);
	rd = (!n->child[1] ? -1 : n->child[1]->depth);
	
	if (ld < rd)
	    while (kdtree_balance(t, n->child[0], level));
	else if (ld > rd)
	    while (kdtree_balance(t, n->child[1], level));

	ld = (!n->child[0] ? -1 : n->child[0]->depth);
	rd = (!n->child[1] ? -1 : n->child[1]->depth);
	n->depth = MAX(ld, rd) + 1;

	dir = (rd > ld);

	top++;
	s[top].n = n->child[dir];
    }
    
    while (top) {
	top--;
	n = s[top].n;

	/* balance node */
	while (kdtree_balance(t, n, level)) {
	    nbal++;
	}
	while (kdtree_balance(t, n->child[0], level));
	while (kdtree_balance(t, n->child[1], level));

	ld = (!n->child[0] ? -1 : n->child[0]->depth);
	rd = (!n->child[1] ? -1 : n->child[1]->depth);
	n->depth = MAX(ld, rd) + 1;

	while (kdtree_balance(t, n, level)) {
	    nbal++;
	}
    }

    while (s[top].n) {
	n = s[top].n;

	/* balance node */
	while (kdtree_balance(t, n, level)) {
	    nbal++;
	}
	while (kdtree_balance(t, n->child[0], level));
	while (kdtree_balance(t, n->child[1], level));

	ld = (!n->child[0] ? -1 : n->child[0]->depth);
	rd = (!n->child[1] ? -1 : n->child[1]->depth);
	n->depth = MAX(ld, rd) + 1;

	while (kdtree_balance(t, n, level)) {
	    nbal++;
	}

	ld = (!n->child[0] ? -1 : n->child[0]->depth);
	rd = (!n->child[1] ? -1 : n->child[1]->depth);

	dir = (rd > ld);

	top++;
	s[top].n = n->child[dir];
    }
    
    while (top) {
	top--;
	n = s[top].n;

	/* update node depth */
	ld = (!n->child[0] ? -1 : n->child[0]->depth);
	rd = (!n->child[1] ? -1 : n->child[1]->depth);
	n->depth = MAX(ld, rd) + 1;
    }

    if (level) {
	top = 0;
	s[top].n = t->root;
	while (s[top].n) {
	    n = s[top].n;

	    /* balance node */
	    while (kdtree_balance(t, n, level)) {
		nbal++;
	    }
	    while (kdtree_balance(t, n->child[0], level));
	    while (kdtree_balance(t, n->child[1], level));

	    ld = (!n->child[0] ? -1 : n->child[0]->depth);
	    rd = (!n->child[1] ? -1 : n->child[1]->depth);
	    n->depth = MAX(ld, rd) + 1;

	    while (kdtree_balance(t, n, level)) {
		nbal++;
	    }

	    diffl = diffr = -1;
	    if (n->child[0]) {
		n2 = n->child[0];
		ld = (!n2->child[0] ? -1 : n2->child[0]->depth);
		rd = (!n2->child[1] ? -1 : n2->child[1]->depth);
		
		diffl = ld - rd;
		if (diffl < 0)
		    diffl = -diffl;
	    }
	    if (n->child[1]) {
		n2 = n->child[1];
		ld = (!n2->child[0] ? -1 : n2->child[0]->depth);
		rd = (!n2->child[1] ? -1 : n2->child[1]->depth);
		
		diffr = ld - rd;
		if (diffr < 0)
		    diffr = -diffr;
	    }
	    
	    dir = (diffr > diffl);

	    top++;
	    s[top].n = n->child[dir];
	}
	
	while (top) {
	    top--;
	    n = s[top].n;

	    /* update node depth */
	    ld = (!n->child[0] ? -1 : n->child[0]->depth);
	    rd = (!n->child[1] ? -1 : n->child[1]->depth);
	    n->depth = MAX(ld, rd) + 1;
	}
    }

    G_debug(1, "k-d tree optimization: %d times balanced, new depth %d",
            nbal, t->root->depth);

    return;
}

/* find k nearest neighbors 
 * results are stored in uid (uids) and d (squared distances)
 * optionally an uid to be skipped can be given
 * useful when searching for the nearest neighbors of an item 
 * that is also in the tree */
int kdtree_knn(struct kdtree *t, double *c, int *uid, double *d, int k, int *skip)
{
    int i, found;
    double diff, dist, maxdist;
    struct kdnode sn, *n;
    struct kdstack {
	struct kdnode *n;
	int dir;
	char v;
    } s[256];
    int dir;
    int top;

    if (!t->root)
	return 0;

    sn.c = c;
    sn.uid = (int)0x80000000;
    if (skip)
	sn.uid = *skip;

    maxdist = 1.0 / 0.0;
    found = 0;

    /* go down */
    top = 0;
    s[top].n = t->root;
    while (s[top].n) {
	n = s[top].n;
	dir = cmp(&sn, n, n->dim) > 0;
	s[top].dir = dir;
	s[top].v = 0;
	top++;
	s[top].n = n->child[dir];
    }
    
    /* go back up */
    while (top) {
	top--;
	
	if (!s[top].v) {
	    s[top].v = 1;
	    n = s[top].n;

	    if (n->uid != sn.uid) {
		if (found < k) {
		    dist = 0.0;
		    i = t->ndims - 1;
		    do {
			diff = sn.c[i] - n->c[i];
			dist += diff * diff;
			
		    } while (i--);

		    i = found;
		    while (i > 0 && d[i - 1] > dist) {
			d[i] = d[i - 1];
			uid[i] = uid[i - 1];
			i--;
		    }
		    if (i < found && d[i] == dist && uid[i] == n->uid)
			G_fatal_error("knn: inserting duplicate");
		    d[i] = dist;
		    uid[i] = n->uid;
		    maxdist = d[found];
		    found++;
		}
		else {
		    dist = 0.0;
		    i = t->ndims - 1;
		    do {
			diff = sn.c[i] - n->c[i];
			dist += diff * diff;
			
		    } while (i-- && dist <= maxdist);

		    if (dist < maxdist) {
			i = k - 1;
			while (i > 0 && d[i - 1] > dist) {
			    d[i] = d[i - 1];
			    uid[i] = uid[i - 1];
			    i--;
			}
			if (d[i] == dist && uid[i] == n->uid)
			    G_fatal_error("knn: inserting duplicate");
			d[i] = dist;
			uid[i] = n->uid;

			maxdist = d[k - 1];
		    }
		}
		if (found == k && maxdist == 0.0)
		    break;
	    }

	    /* look on the other side ? */
	    dir = s[top].dir;
	    diff = sn.c[(int)n->dim] - n->c[(int)n->dim];
	    dist = diff * diff;

	    if (dist <= maxdist) {
		/* go down the other side */
		top++;
		s[top].n = n->child[!dir];
		while (s[top].n) {
		    n = s[top].n;
		    dir = cmp(&sn, n, n->dim) > 0;
		    s[top].dir = dir;
		    s[top].v = 0;
		    top++;
		    s[top].n = n->child[dir];
		}
	    }
	}
    }

    return found;
}

/* find all nearest neighbors within distance aka radius search
 * results are stored in puid (uids) and pd (squared distances)
 * memory is allocated as needed, the calling fn must free the memory
 * optionally an uid to be skipped can be given */
int kdtree_dnn(struct kdtree *t, double *c, int **puid, double **pd,
               double maxdist, int *skip)
{
    int i, k, found;
    double diff, dist;
    struct kdnode sn, *n;
    struct kdstack {
	struct kdnode *n;
	int dir;
	char v;
    } s[256];
    int dir;
    int top;
    int *uid;
    double *d, maxdistsq;

    if (!t->root)
	return 0;

    sn.c = c;
    sn.uid = (int)0x80000000;
    if (skip)
	sn.uid = *skip;

    *pd = NULL;
    *puid = NULL;

    k = 0;
    uid = NULL;
    d = NULL;

    found = 0;
    maxdistsq = maxdist * maxdist;

    /* go down */
    top = 0;
    s[top].n = t->root;
    while (s[top].n) {
	n = s[top].n;
	dir = cmp(&sn, n, n->dim) > 0;
	s[top].dir = dir;
	s[top].v = 0;
	top++;
	s[top].n = n->child[dir];
    }
    
    /* go back up */
    while (top) {
	top--;
	
	if (!s[top].v) {
	    s[top].v = 1;
	    n = s[top].n;

	    if (n->uid != sn.uid) {
		dist = 0;
		i = t->ndims - 1;
		do {
		    diff = sn.c[i] - n->c[i];
		    dist += diff * diff;
		    
		} while (i-- && dist <= maxdistsq);

		if (dist <= maxdistsq) {
		    if (found + 1 >= k) {
			k = found + 10;
			uid = G_realloc(uid, k * sizeof(int));
			d = G_realloc(d, k * sizeof(double));
		    }
		    i = found;
		    while (i > 0 && d[i - 1] > dist) {
			d[i] = d[i - 1];
			uid[i] = uid[i - 1];
			i--;
		    }
		    if (i < found && d[i] == dist && uid[i] == n->uid)
			G_fatal_error("dnn: inserting duplicate");
		    d[i] = dist;
		    uid[i] = n->uid;
		    found++;
		}
	    }

	    /* look on the other side ? */
	    dir = s[top].dir;

	    diff = fabs(sn.c[(int)n->dim] - n->c[(int)n->dim]);
	    if (diff <= maxdist) {
		/* go down the other side */
		top++;
		s[top].n = n->child[!dir];
		while (s[top].n) {
		    n = s[top].n;
		    dir = cmp(&sn, n, n->dim) > 0;
		    s[top].dir = dir;
		    s[top].v = 0;
		    top++;
		    s[top].n = n->child[dir];
		}
	    }
	}
    }

    *pd = d;
    *puid = uid;

    return found;
}

/* find all nearest neighbors within range aka box search
 * the range is specified with min and max for each dimension as
 * (min1, min2, ..., minn, max1, max2, ..., maxn)
 * results are stored in puid (uids)
 * memory is allocated as needed, the calling fn must free the memory
 * optionally an uid to be skipped can be given */
int kdtree_rnn(struct kdtree *t, double *c, int **puid, int *skip)
{
    int i, k, found, inside;
    struct kdnode sn, *n;
    struct kdstack {
	struct kdnode *n;
	int dir;
	char v;
    } s[256];
    int dir;
    int top;
    int *uid;

    if (!t->root)
	return 0;

    sn.c = c;
    sn.uid = (int)0x80000000;
    if (skip)
	sn.uid = *skip;

    *puid = NULL;

    k = 0;
    uid = NULL;

    found = 0;

    /* go down */
    top = 0;
    s[top].n = t->root;
    while (s[top].n) {
	n = s[top].n;
	dir = cmp(&sn, n, n->dim) > 0;
	s[top].dir = dir;
	s[top].v = 0;
	top++;
	s[top].n = n->child[dir];
    }
    
    /* go back up */
    while (top) {
	top--;
	
	if (!s[top].v) {
	    s[top].v = 1;
	    n = s[top].n;

	    if (n->uid != sn.uid) {
		inside = 1;
		for (i = 0; i < t->ndims; i++) {
		    if (n->c[i] < sn.c[i] || n->c[i] > sn.c[i + t->ndims]) {
			inside = 0;
			break;
		    }
		}

		if (inside) {
		    if (found + 1 >= k) {
			k = found + 10;
			uid = G_realloc(uid, k * sizeof(int));
		    }
		    i = found;
		    uid[i] = n->uid;
		    found++;
		}
	    }

	    /* look on the other side ? */
	    dir = s[top].dir;
	    if (n->c[(int)n->dim] >= sn.c[(int)n->dim] && 
	        n->c[(int)n->dim] <= sn.c[(int)n->dim + t->ndims]) {
		/* go down the other side */
		top++;
		s[top].n = n->child[!dir];
		while (s[top].n) {
		    n = s[top].n;
		    dir = cmp(&sn, n, n->dim) > 0;
		    s[top].dir = dir;
		    s[top].v = 0;
		    top++;
		    s[top].n = n->child[dir];
		}
	    }
	}
    }

    *puid = uid;

    return found;
}

/* initialize tree traversal
 * (re-)sets trav structure
 * returns 0
 */
int kdtree_init_trav(struct kdtrav *trav, struct kdtree *tree)
{
    trav->tree = tree;
    trav->curr_node = tree->root;
    trav->first = 1;
    trav->top = 0;

    return 0;
}

/* traverse the tree
 * useful to get all items in the tree non-recursively
 * struct kdtrav *trav needs to be initialized first
 * returns 1, 0 when finished
 */
int kdtree_traverse(struct kdtrav *trav, double *c, int *uid)
{
    if (trav->curr_node == NULL) {
	if (trav->first)
	    G_debug(1, "k-d tree: empty tree");
	else
	    G_debug(1, "k-d tree: finished traversing");

	return 0;
    }

    if (trav->first) {
	trav->first = 0;
	return kdtree_first(trav, c, uid);
    }

    return kdtree_next(trav, c, uid);
}


/**********************************************/
/*            internal functions              */
/**********************************************/

static int kdtree_replace(struct kdtree *t, struct kdnode *r)
{
    double mindist;
    int rdir, ordir, dir;
    int ld, rd;
    struct kdnode *n, *rn, *or;
    struct kdstack {
	struct kdnode *n;
	int dir;
	char v;
    } s[256];
    int top, top2;
    int is_leaf;
    int nr;

    if (!r)
	return 0;
    if (!r->child[0] && !r->child[1])
	return 0;

    /* do not call kdtree_balance in this fn, this can cause 
     * stack overflow due to too many recursive calls */

    /* find replacement for r
     * overwrite r, delete replacement */
    nr = 0;

    /* pick a subtree */
    rdir = 1;

    or = r;
    ld = (!or->child[0] ? -1 : or->child[0]->depth);
    rd = (!or->child[1] ? -1 : or->child[1]->depth);

    if (ld > rd) {
	rdir = 0;
    }

    /* replace old root, make replacement the new root
     * repeat until replacement is leaf */
    ordir = rdir;
    is_leaf = 0;
    s[0].n = or;
    s[0].dir = ordir;
    top2 = 1;
    mindist = -1;
    while (!is_leaf) {
	rn = NULL;

	/* find replacement for old root */
	top = top2;
	s[top].n = or->child[ordir];

	n = s[top].n;
	rn = n;
	mindist = or->c[(int)or->dim] - n->c[(int)or->dim];
	if (ordir)
	    mindist = -mindist;

	/* go down */
	while (s[top].n) {
	    n = s[top].n;
	    dir = !ordir;
	    if (n->dim != or->dim)
		dir = cmp(or, n, n->dim) > 0;
	    s[top].dir = dir;
	    s[top].v = 0;
	    top++;
	    s[top].n = n->child[dir];
	}

	/* go back up */
	while (top > top2) {
	    top--;

	    if (!s[top].v) {
		s[top].v = 1;
		n = s[top].n;
		if ((cmp(rn, n, or->dim) > 0) == ordir) {
		    rn = n;
		    mindist = or->c[(int)or->dim] - n->c[(int)or->dim];
		    if (ordir)
			mindist = -mindist;
		}

		/* look on the other side ? */
		dir = s[top].dir;
		if (n->dim != or->dim &&
		    mindist >= fabs(n->c[(int)n->dim] - n->c[(int)n->dim])) {
		    /* go down the other side */
		    top++;
		    s[top].n = n->child[!dir];
		    while (s[top].n) {
			n = s[top].n;
			dir = !ordir;
			if (n->dim != or->dim)
			    dir = cmp(or, n, n->dim) > 0;
			s[top].dir = dir;
			s[top].v = 0;
			top++;
			s[top].n = n->child[dir];
		    }
		}
	    }
	}

#ifdef KD_DEBUG
	if (!rn)
	    G_fatal_error("No replacement");
	if (ordir && or->c[(int)or->dim] > rn->c[(int)or->dim])
	    G_fatal_error("rn is smaller");

	if (!ordir && or->c[(int)or->dim] < rn->c[(int)or->dim])
	    G_fatal_error("rn is larger");

	if (or->child[1]) {
	    dir = cmp(or->child[1], rn, or->dim);
	    if (dir < 0) {
		int i;

		for (i = 0; i < t->ndims; i++)
		    G_message("rn c %g, or child c %g",
			    rn->c[i], or->child[1]->c[i]);
		G_fatal_error("Right child of old root is smaller than rn, dir is %d", ordir);
	    }
	}
	if (or->child[0]) {
	    dir = cmp(or->child[0], rn, or->dim);
	    if (dir > 0) {
		int i;

		for (i = 0; i < t->ndims; i++)
		    G_message("rn c %g, or child c %g",
			    rn->c[i], or->child[0]->c[i]);
		G_fatal_error("Left child of old root is larger than rn, dir is %d", ordir);
	    }
	}
#endif

	is_leaf = (rn->child[0] == NULL && rn->child[1] == NULL);

#ifdef KD_DEBUG
	if (is_leaf && rn->depth != 0)
	    G_fatal_error("rn is leaf but depth is %d", (int)rn->depth);
	if (!is_leaf && rn->depth <= 0)
	    G_fatal_error("rn is not leaf but depth is %d", (int)rn->depth);
#endif

	nr++;

	/* go to replacement from or->child[ordir] */
	top = top2;
	dir = 1;
	while (dir) {
	    n = s[top].n;
	    dir = cmp(rn, n, n->dim);
	    if (dir) {
		s[top].dir = dir > 0;
		top++;
		s[top].n = n->child[dir > 0];

		if (!s[top].n) {
		    G_fatal_error("(Last) replacement disappeared %d", nr);
		}
	    }
	}

#ifdef KD_DEBUG
	if (s[top].n != rn)
	    G_fatal_error("rn is unreachable from or");
#endif

	top2 = top;
	s[top2 + 1].n = NULL;

	/* copy replacement to old root */
	memcpy(or->c, rn->c, t->csize);
	or->uid = rn->uid;
	
	if (!is_leaf) {
	    /* make replacement the old root */
	    or = rn;

	    /* pick a subtree */
	    ordir = 1;
	    ld = (!or->child[0] ? -1 : or->child[0]->depth);
	    rd = (!or->child[1] ? -1 : or->child[1]->depth);
	    if (ld > rd) {
		ordir = 0;
	    }
	    s[top2].dir = ordir;
	    top2++;
	}
    }

    if (!rn)
	G_fatal_error("No replacement at all");

    /* delete last replacement */
    if (s[top2].n != rn) {
	G_fatal_error("Wrong top2 for last replacement");
    }
    top = top2 - 1;
    n = s[top].n;
    dir = s[top].dir;
    if (n->child[dir] != rn) {
	G_fatal_error("Last replacement disappeared");
    }
    kdtree_free_node(rn);
    n->child[dir] = NULL;
    t->count--;

    kdtree_update_node(t, n);
    top++;

    /* go back up */
    while (top) {
	top--;
	n = s[top].n;

#ifdef KD_DEBUG
	/* debug directions */
	if (n->child[0]) {
	    if (cmp(n->child[0], n, n->dim) > 0)
		G_warning("Left child is larger");
	}
	if (n->child[1]) {
	    if (cmp(n->child[1], n, n->dim) < 1)
		G_warning("Right child is not larger");
	}
#endif

	/* update node */
	kdtree_update_node(t, n);
    }

    return nr;
}

static int kdtree_balance(struct kdtree *t, struct kdnode *r, int bmode)
{
    struct kdnode *or;
    int dir;
    int rd, ld;
    int old_depth;
    int btol;

    if (!r) {
	return 0;
    }

    ld = (!r->child[0] ? -1 : r->child[0]->depth);
    rd = (!r->child[1] ? -1 : r->child[1]->depth);
    old_depth = MAX(ld, rd) + 1;
    
    if (old_depth != r->depth) {
	G_warning("balancing: depth is wrong: %d != %d", r->depth, old_depth);
	kdtree_update_node(t, r);
    }

    /* subtree difference */
    btol = t->btol;
    if (!r->child[0] || !r->child[1])
	btol = 2;
    dir = -1;
    ld = (!r->child[0] ? -1 : r->child[0]->depth);
    rd = (!r->child[1] ? -1 : r->child[1]->depth);
    if (ld > rd + btol) {
	dir = 0;
    }
    else if (rd > ld + btol) {
	dir = 1;
    }
    else {
	return 0;
    }

    or = kdtree_newnode(t);
    memcpy(or->c, r->c, t->csize);
    or->uid = r->uid;
    or->dim = t->nextdim[r->dim];

    if (!kdtree_replace(t, r))
	G_fatal_error("kdtree_balance: nothing replaced");

#ifdef KD_DEBUG
    if (!cmp(r, or, r->dim)) {
	G_warning("kdtree_balance: replacement failed");
	kdtree_free_node(or);
	
	return 0;
    }
#endif

    r->child[!dir] = kdtree_insert2(t, r->child[!dir], or, bmode, 1); /* bmode */

    /* update node */
    kdtree_update_node(t, r);

    if (r->depth == old_depth) {
	G_debug(4, "balancing had no effect");
	return 1;
    }

    if (r->depth > old_depth)
	G_fatal_error("balancing failed");

    return 1;
}

static struct kdnode *kdtree_insert2(struct kdtree *t, struct kdnode *r,
                                     struct kdnode *nnew,
				     int balance, int dc)
{
    struct kdnode *n;
    struct kdstack {
	struct kdnode *n;
	int dir;
    } s[256];
    int top;
    int dir;
    int bmode;

    if (!r) {
	r = nnew;
	t->count++;

	return r;
    }

    /* level of recursion */
    rcalls++;
    if (rcallsmax < rcalls)
	rcallsmax = rcalls;

    /* balancing modes
     * bmode = 0: no recursion (only insert -> balance -> insert)
     *            slower, higher tree depth
     * bmode = 1: recursion (insert -> balance -> insert -> balance ...)
     *            faster, more compact tree
     *  */
    bmode = 1;

    /* find node with free child */
    top = 0;
    s[top].n = r;
    while (s[top].n) {

	n = s[top].n;

	if (!cmpc(nnew, n, t) && (!dc || nnew->uid == n->uid)) {

	    G_debug(1, "KD node exists already, nothing to do");
	    kdtree_free_node(nnew);

	    if (!balance) {
		rcalls--;
		return r;
	    }

	    break;
	}
	dir = cmp(nnew, n, n->dim) > 0;
	s[top].dir = dir;

	top++;
	if (top > 255)
	    G_fatal_error("depth too large: %d", top);
	s[top].n = n->child[dir];
    }

    if (!s[top].n) {
	/* insert to child pointer of parent */
	top--;
	n = s[top].n;
	dir = s[top].dir;
	n->child[dir] = nnew;
	nnew->dim = t->nextdim[n->dim];

	t->count++;
	top++;
    }

    /* go back up */
    while (top) {
	top--;
	n = s[top].n;

	/* update node */
	kdtree_update_node(t, n);

	/* do not balance on the way back up */

#ifdef KD_DEBUG
	/* debug directions */
	if (n->child[0]) {
	    if (cmp(n->child[0], n, n->dim) > 0)
		G_warning("Insert2: Left child is larger");
	}
	if (n->child[1]) {
	    if (cmp(n->child[1], n, n->dim) < 1)
		G_warning("Insert2: Right child is not larger");
	}
#endif
    }

    if (balance) {
	int iter, bmode2;

	/* fix any inconsistencies in the (sub-)tree */
	iter = 0;
	bmode2 = 0;
	top = 0;
	s[top].n = r;
	while (top >= 0) {

	    n = s[top].n;

	    /* top-down balancing
	     * slower but more compact */
	    if (!bmode2) {
		while (kdtree_balance(t, n, bmode));
	    }

	    /* go down */
	    if (n->child[0] && n->child[0]->balance) {
		dir = 0;
		top++;
		s[top].n = n->child[dir];
	    }
	    else if (n->child[1] && n->child[1]->balance) {
		dir = 1;
		top++;
		s[top].n = n->child[dir];
	    }
	    /* go back up */
	    else {

		/* bottom-up balancing
		 * faster but less compact */
		if (bmode2) {
		    while (kdtree_balance(t, n, bmode));
		}
		top--;
		if (top >= 0) {
		    kdtree_update_node(t, s[top].n);
		}
		if (!bmode2 && top == 0) {
		    iter++;
		    if (iter == 2) {
			/* the top node has been visited twice, 
			 * switch from top-down to bottom-up balancing */
			iter = 0;
			bmode2 = 1;
		    }
		}
	    }
	}
    }

    rcalls--;

    return r;
}

/* start traversing the tree
 * returns pointer to first item
 */
static int kdtree_first(struct kdtrav *trav, double *c, int *uid)
{
    /* get smallest item */
    while (trav->curr_node->child[0] != NULL) {
	trav->up[trav->top++] = trav->curr_node;
	trav->curr_node = trav->curr_node->child[0];
    }

    memcpy(c, trav->curr_node->c, trav->tree->csize);
    *uid = trav->curr_node->uid;

    return 1;
}

/* continue traversing the tree in ascending order
 * returns pointer to data item, NULL when finished
 */
static int kdtree_next(struct kdtrav *trav, double *c, int *uid)
{
    if (trav->curr_node->child[1] != NULL) {
	/* something on the right side: larger item */
	trav->up[trav->top++] = trav->curr_node;
	trav->curr_node = trav->curr_node->child[1];

	/* go down, find smallest item in this branch */
	while (trav->curr_node->child[0] != NULL) {
	    trav->up[trav->top++] = trav->curr_node;
	    trav->curr_node = trav->curr_node->child[0];
	}
    }
    else {
	/* at smallest item in this branch, go back up */
	struct kdnode *last;

	do {
	    if (trav->top == 0) {
		trav->curr_node = NULL;
		break;
	    }
	    last = trav->curr_node;
	    trav->curr_node = trav->up[--trav->top];
	} while (last == trav->curr_node->child[1]);
    }

    if (trav->curr_node != NULL) {
	memcpy(c, trav->curr_node->c, trav->tree->csize);
	*uid = trav->curr_node->uid;

	return 1;
    }

    return 0;		/* finished traversing */
}
