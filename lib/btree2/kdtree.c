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

#define KD_BTOL 6

#ifdef KD_DEBUG
#undef KD_DEBUG
#endif

static struct kdnode *kdtree_insert2(struct kdtree *, struct kdnode *,
                                 struct kdnode *, int, int);
static int kdtree_replace(struct kdtree *, struct kdnode *);
static struct kdnode *kdtree_balance(struct kdtree *, struct kdnode *);

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

/* create a new kd tree with ndims dimensions,
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

/* insert an item (coordinates c and uid) into the kd tree
 * dc == 1: allow duplicate coordinates */
int kdtree_insert(struct kdtree *t, double *c, int uid, int dc)
{
    struct kdnode *nnew;
    size_t count = t->count;

    nnew = kdtree_newnode(t);
    memcpy(nnew->c, c, t->csize);
    nnew->uid = uid;

    t->root = kdtree_insert2(t, t->root, nnew, 1, dc);

    return count < t->count;
}

/* remove an item from the kd tree
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
    int ld, rd;

    sn.c = c;
    sn.uid = uid;

    /* find sn node */
    top = 0;
    s[top].n = t->root;
    dir = 1;
    found = 0;
    while (found) {
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
    if (!kdtree_replace(t, s[top].n)) {
	t->root = NULL;

	return 1;
    }
    if (top) {
	top--;
	dir = s[top].dir;
	n = s[top].n;
	n->child[dir] = kdtree_balance(t, n->child[dir]);

	/* update node depth */
	ld = rd = -1;
	if (n->child[0])
	    ld = n->child[0]->depth;
	if (n->child[1])
	    rd = n->child[1]->depth;
	n->depth = MAX(ld, rd) + 1;
    }
    while (top) {
	top--;
	n = s[top].n;

	/* update node depth */
	ld = rd = -1;
	if (n->child[0])
	    ld = n->child[0]->depth;
	if (n->child[1])
	    rd = n->child[1]->depth;
	n->depth = MAX(ld, rd) + 1;
    }

    t->root = kdtree_balance(t, t->root);

    return 1;
}

/* find k nearest neighbors 
 * results are stored in uid and d (distances)
 * optionally an uid to be skipped can be given
 * useful when searching for the nearest neighbor of an item 
 * that is also in the tree */
int kdtree_knn(struct kdtree *t, double *c, int *uid, double *d, int k, int *skip)
{
    int i, found;
    double diff, dist, maxdist;
    struct kdnode sn, *n, *r;
    struct kdstack {
	struct kdnode *n;
	int dir;
	char v;
    } s[256];
    int dir;
    int top;

    r = t->root;
    sn.c = c;
    sn.uid = (int)0x80000000;
    if (skip)
	sn.uid = *skip;
    
    top = 0;
    s[top].n = r;
    if (!s[top].n)
	return 0;

    maxdist = 1.0 / 0.0;
    found = 0;

    n = s[top].n;
    if (n->uid != sn.uid) {
	maxdist = 0;
	for (i = 0; i < t->ndims; i++) {
	    diff = sn.c[i] - n->c[i];
	    maxdist += diff * diff;
	}
	d[0] = maxdist;
	uid[0] = n->uid;
	found = 1;
    }

    /* go down */
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
	n = s[top].n;
	
	if (!s[top].v) {
	    s[top].v = 1;

	    if (n->uid != sn.uid) {
		dist = 0;
		for (i = 0; i < t->ndims; i++) {
		    diff = sn.c[i] - n->c[i];
		    dist += diff * diff;
		    if (found == k && dist > maxdist)
			break;
		}
		if (found < k) {
		    i = found;
		    while (i > 0 && d[i - 1] > dist) {
			d[i] = d[i - 1];
			uid[i] = uid[i - 1];
			i--;
		    }
		    if (d[i] == dist && uid[i] == n->uid)
			G_fatal_error("knn: inserting duplicate");
		    d[i] = dist;
		    uid[i] = n->uid;
		    maxdist = d[found];
		    found++;
		}
		else if (dist < maxdist) {
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

    for (i = 0; i < found; i++)
	d[i] = sqrt(d[i]);

    return found;
}

/* find all nearest neighbors within distance
 * results are stored in puid and pd (distances)
 * memory is allocated as needed, the calling fn must free the memory
 * optionally an uid to be skipped can be given */
int kdtree_dnn(struct kdtree *t, double *c, int **puid, double **pd, double maxdist, int *skip)
{
    int i, k, found;
    double diff, dist;
    struct kdnode sn, *n, *r;
    struct kdstack {
	struct kdnode *n;
	int dir;
	char v;
    } s[256];
    int dir;
    int top;
    int *uid;
    double *d;

    r = t->root;
    sn.c = c;
    sn.uid = (int)0x80000000;
    if (skip)
	sn.uid = *skip;

    *pd = NULL;
    *puid = NULL;

    top = 0;
    s[top].n = r;
    if (!s[top].n)
	return 0;

    k = 0;
    uid = NULL;
    d = NULL;

    found = 0;
    maxdist = maxdist * maxdist;

    /* go down */
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
	n = s[top].n;
	
	if (!s[top].v && n->uid != sn.uid) {
	    dist = 0;
	    for (i = 0; i < t->ndims; i++) {
		diff = sn.c[i] - n->c[i];
		dist += diff * diff;
		if (found == k && dist > maxdist)
		    break;
	    }
	    if (dist <= maxdist) {
		if (found + 1 >= k) {
		    k += 10;
		    uid = G_realloc(uid, k * sizeof(int));
		    d = G_realloc(d, k * sizeof(double));
		}
		i = found;
		while (i > 0 && d[i - 1] > dist) {
		    d[i] = d[i - 1];
		    uid[i] = uid[i - 1];
		    i--;
		}
		if (d[i] == dist && uid[i] == n->uid)
		    G_fatal_error("knn: inserting duplicate");
		d[i] = dist;
		uid[i] = n->uid;
		found++;
	    }
	}

	/* look on the other side ? */
	if (!s[top].v) {
	    s[top].v = 1;
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
    
    for (i = 0; i < found; i++)
	d[i] = sqrt(d[i]);

    *pd = d;
    *puid = uid;

    return found;
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

    if (!or->child[rdir]) {
	 /* no replacement, delete */
	kdtree_free_node(r);

	return nr;
    }

    /* replace old root, make replacement the new root
     * repeat until replacement is leaf */
    ordir = rdir;
    is_leaf = 0;
    top2 = 0;
    mindist = -1;
    while (!is_leaf) {
	rn = NULL;

	/* find replacement for old root */
	top = top2;
	s[top].n = or->child[ordir];
	if (!s[top].n)
	    break;

	n = s[top].n;
	rn = n;
	mindist = fabs(or->c[(int)or->dim] - n->c[(int)or->dim]);

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
		    mindist = fabs(or->c[(int)or->dim] - n->c[(int)or->dim]);
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
		G_debug(0, "rn c %g, or child c %g",
		        rn->c[i], or->child[1]->c[i]);
		G_fatal_error("Right or child is smaller than rn, dir is %d", ordir);
	    }
	}
	if (or->child[0]) {
	    dir = cmp(or->child[0], rn, or->dim);
	    if (dir > 0) {
		int i;
		for (i = 0; i < t->ndims; i++)
		G_debug(0, "rn c %g, or child c %g",
		        rn->c[i], or->child[0]->c[i]);
		G_fatal_error("Left or child is larger than rn, dir is %d", ordir);
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
	if (s[top].n != rn)
	    G_fatal_error("rn is unreachable from or");

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

    /* delete replacement */
    top = top2;
    if (top) {
	int old_depth;

	n = s[top - 1].n;
	dir = 0;
	if (n->child[dir] != rn) {
	    dir = !dir;
	}
	if (n->child[dir] != rn) {
	    G_fatal_error("Last replacement disappeared");
	}
	n->child[dir] = NULL;

#ifndef KD_DEBUG
	old_depth = n->depth;
	n->depth = (!n->child[!dir] ? 0 : n->child[!dir]->depth + 1);
	top--;
	if (n->depth == old_depth)
	    top = 0;
#endif

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

	    /* update depth */
	    ld = (!n->child[0] ? -1 : n->child[0]->depth);
	    rd = (!n->child[1] ? -1 : n->child[1]->depth);
	    n->depth = MAX(ld, rd) + 1;
	}
    }
    else {
	r->child[rdir] = NULL;
    }
    kdtree_free_node(rn);
    t->count--;

    /* update depth */
    ld = (!r->child[0] ? -1 : r->child[0]->depth);
    rd = (!r->child[1] ? -1 : r->child[1]->depth);
    r->depth = MAX(ld, rd) + 1;

    return nr;
}

static struct kdnode *kdtree_balance(struct kdtree *t, struct kdnode *r)
{
    struct kdnode *or;
    int dir;
    int rd, ld;

    if (!r)
	return NULL;

    /* subtree difference */
    dir = -1;
    ld = (!r->child[0] ? -1 : r->child[0]->depth);
    rd = (!r->child[1] ? -1 : r->child[1]->depth);
    if (ld > rd + t->btol) {
	dir = 0;
    }
    else if (rd > ld + t->btol) {
	dir = 1;
    }
    else
	return r;

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
	
	return r;
    }
#endif

    r->child[!dir] = kdtree_insert2(t, r->child[!dir], or, 0, 1);

    /* update node depth */
    ld = r->child[0]->depth;
    rd = r->child[1]->depth;
    r->depth = MAX(ld, rd) + 1;

    return r;
}

static struct kdnode *kdtree_insert2(struct kdtree *t, struct kdnode *r,
                                 struct kdnode *nnew, int balance, int dc)
{
    struct kdnode *n;
    struct kdstack {
	struct kdnode *n;
	int dir;
    } s[256];
    int top;
    int dir;
    int ld, rd;
    int old_depth;
    int go_back;

    if (!r) {
	r = nnew;
	t->count++;

	return r;
    }

    if (balance) {
	/* balance root */
	r = kdtree_balance(t, r);
    }

    /* find node with free child */
    top = 0;
    go_back = 0;
    s[top].n = r;
    while (s[top].n) {
	n = s[top].n;
	if (!cmpc(nnew, n, t) && (!dc || nnew->uid == n->uid)) {

	    G_debug(1, "KD node exists already, nothing to do");
	    kdtree_free_node(nnew);

	    return r;
	}
	dir = cmp(nnew, n, n->dim) > 0;
	s[top].dir = dir;

	if (balance) {
	    /* balance left subtree */
	    n->child[0] = kdtree_balance(t, n->child[0]);
	    /* balance right subtree */
	    n->child[1] = kdtree_balance(t, n->child[1]);
	}
	old_depth = n->depth;
	/* update node depth */
	ld = (!n->child[0] ? -1 : n->child[0]->depth);
	rd = (!n->child[1] ? -1 : n->child[1]->depth);
	n->depth = MAX(ld, rd) + 1;
	if (old_depth != n->depth)
	    go_back = top;

	top++;
	if (top > 255)
	    G_fatal_error("depth too large: %d", top);
	s[top].n = n->child[dir];
    }

    /* insert, update child pointer of parent */
    s[top].n = nnew;
    top--;
    dir = s[top].dir;
    s[top].n->child[dir] = nnew;
    nnew->dim = t->nextdim[s[top].n->dim];

    old_depth = s[top].n->depth;
    s[top].n->depth = (!s[top].n->child[!dir] ? 1 : s[top].n->child[!dir]->depth + 1);

    if (old_depth != s[top].n->depth)
	go_back = top;

    t->count++;

    /* go back up */
#ifndef KD_DEBUG
    top = go_back;
#endif

    while (top) {
	top--;
	n = s[top].n;

#ifdef KD_DEBUG
	/* debug directions */
	if (n->child[0]) {
	    if (cmp(n->child[0], n, n->dim) > 0)
		G_warning("Insert before balance: Left child is larger");
	}
	if (n->child[1]) {
	    if (cmp(n->child[1], n, n->dim) < 1)
		G_warning("Insert before balance: Right child is not larger");
	}
#endif

	/* update node depth */
	ld = (!n->child[0] ? -1 : n->child[0]->depth);
	rd = (!n->child[1] ? -1 : n->child[1]->depth);
	n->depth = MAX(ld, rd) + 1;
    }

    return r;
}
