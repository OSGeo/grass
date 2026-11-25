/*!
 * \file kdtree.c
 *
 * \brief binary search tree
 *
 * Dynamic balanced k-d tree implementation
 *
 * SPDX-FileCopyrightText: 2014 GRASS Development Team
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * \author Markus Metz
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "kdtree.h"

#define KD_BTOL 7

#ifdef KD_DEBUG
#undef KD_DEBUG
#endif

static int rcalls = 0;
static int rcallsmax = 0;

/* comparison modes used while building a tree from an array of points */
#define KD_CMP_KEY 0 /* all coordinates, then the uid if dc is set */
#define KD_CMP_DIM 1 /* coordinate of one dimension, then the uid */
#define KD_CMP_IDX 2 /* index of the point */

/* context for building a tree from an array of points */
struct kdbuild {
    const double
        *coords;     /* coordinates of the points, ndims values per point */
    const int *uids; /* unique ids of the points */
    int ndims;       /* number of dimensions */
    int dc;          /* allow duplicate coordinates */
};

/* forward declarations of the internal functions */
static struct kdnode *kd_insert2(struct kdtree *, struct kdnode *,
                                 struct kdnode *, int, int);
static struct kdnode *kd_newnode(struct kdtree *t);
static void kd_free_node(struct kdtree *t, struct kdnode *n);
static void kd_update_node(struct kdtree *t, struct kdnode *n);
static void kd_swap(int *a, int *b);
static int kd_same_point(const struct kdbuild *b, int i, int j);
static void kd_sort(struct kdbuild *b, int *idx, int lo, int hi, int mode,
                    int dim);
static int kd_select(struct kdbuild *b, int *idx, int lo, int hi, int k,
                     int dim);
static struct kdnode *kd_build(struct kdtree *t, struct kdbuild *b, int *idx,
                               int lo, int hi, int dim);
static int kd_replace(struct kdtree *, struct kdnode *);
static int kd_balance(struct kdtree *, struct kdnode *, int);
static int kd_first(struct kdtrav *, double *, int *);
static int kd_next(struct kdtrav *, double *, int *);

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

/* compare the coordinates of two points */
static int cmp_point_coords(const struct kdbuild *b, int i, int j)
{
    int d;

    for (d = 0; d < b->ndims; d++) {
        double ci = b->coords[(size_t)i * b->ndims + d];
        double cj = b->coords[(size_t)j * b->ndims + d];

        if (ci != cj)
            return (ci < cj ? -1 : 1);
    }

    return 0;
}

/* compare two points by the coordinate of one dimension, then by the uid
 * this is the order which is used by the tree search */
static int cmp_point_order(const struct kdbuild *b, int i, int j, int dim)
{
    double ci = b->coords[(size_t)i * b->ndims + dim];
    double cj = b->coords[(size_t)j * b->ndims + dim];

    if (ci != cj)
        return (ci < cj ? -1 : 1);
    if (b->uids[i] != b->uids[j])
        return (b->uids[i] < b->uids[j] ? -1 : 1);

    return 0;
}

/* compare two points
 * KD_CMP_KEY: all coordinates, then the uid if dc is set
 * KD_CMP_DIM: coordinate of one dimension, then the uid
 * KD_CMP_IDX: index of the point
 * the index of the point is the last tie breaker, it makes the order a strict
 * total order */
static int cmp_points(const struct kdbuild *b, int i, int j, int mode, int dim)
{
    int c;

    if (mode == KD_CMP_IDX)
        return (i < j ? -1 : i > j);

    if (mode == KD_CMP_DIM)
        c = cmp_point_order(b, i, j, dim);
    else {
        c = cmp_point_coords(b, i, j);
        if (!c && b->dc && b->uids[i] != b->uids[j])
            c = (b->uids[i] < b->uids[j] ? -1 : 1);
    }

    if (c)
        return c;

    return (i < j ? -1 : i > j);
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
    t->nodes = NULL;
    t->pool = NULL;
    t->nnodes = 0;

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
    while ((it = save) != NULL) {
        if (it->child[0] == NULL) {
            /* No left links, just kill the node and move on */
            save = it->child[1];
            kd_free_node(t, it);
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
    t->count = 0;

    if (t->pool) {
        G_free(t->pool);
        t->pool = NULL;
    }
    if (t->nodes) {
        G_free(t->nodes);
        t->nodes = NULL;
    }
    t->nnodes = 0;
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

    nnew = kd_newnode(t);
    memcpy(nnew->c, c, t->csize);
    nnew->uid = uid;

    /* insert the item into the tree, kd_insert2() is recursive */
    t->root = kd_insert2(t, t->root, nnew, 1, dc);

    /* print depth of recursion
     * recursively called fns are insert2, balance, and replace */
    /*
       if (rcallsmax > 1)
       fprintf(stdout, "%d\n", rcallsmax);
     */

    return count < t->count;
}

/* create a new k-d tree from an array of points
 * the tree is built by splitting the points at their median, which is much
 * faster than inserting the points one by one and needs less memory
 * the uids array is modified in place, it contains the uids of the points
 * which are in the tree when the function returns */
struct kdtree *kdtree_create_from_points(char ndims, const double *coords,
                                         int *uids, int n, int dc)
{
    struct kdtree *t;
    struct kdbuild b;
    int *idx;
    int m, i, j;

    if (ndims < 1)
        G_fatal_error(
            "kdtree_create_from_points: invalid number of dimensions");
    if (n < 0)
        G_fatal_error("kdtree_create_from_points: invalid number of points");

    t = kdtree_create(ndims, NULL);

    if (n == 0)
        return t;

    idx = G_malloc(sizeof(int) * (size_t)n);
    for (i = 0; i < n; i++)
        idx[i] = i;

    b.coords = coords;
    b.uids = uids;
    b.ndims = ndims;
    b.dc = dc;

    /* sort the points to find the duplicates */
    kd_sort(&b, idx, 0, n, KD_CMP_KEY, 0);

    /* remove the duplicates, the first point of a set of identical points is
     * kept, just like kdtree_insert() does */
    m = 0;
    for (i = 0; i < n;) {
        for (j = i + 1; j < n && kd_same_point(&b, idx[i], idx[j]); j++)
            ;
        idx[m++] = idx[i];
        i = j;
    }

    /* restore the order of the input array */
    kd_sort(&b, idx, 0, m, KD_CMP_IDX, 0);

    /* allocate the coordinates and the nodes of the tree */
    t->pool = G_malloc(sizeof(double) * (size_t)m * ndims);
    t->nodes = G_malloc(sizeof(struct kdnode) * (size_t)m);
    t->nnodes = m;

    /* copy the coordinates of the points and remove the uids of the
     * duplicates from the uids array */
    for (i = 0; i < m; i++) {
        int p = idx[i];

        memcpy(t->pool + (size_t)i * ndims, coords + (size_t)p * ndims,
               (size_t)ndims * sizeof(double));
        uids[i] = uids[p];
    }

    /* initialize the nodes */
    for (i = 0; i < m; i++) {
        struct kdnode *node = &t->nodes[i];

        node->c = t->pool + (size_t)i * ndims;
        node->uid = uids[i];
        node->dim = 0;
        node->depth = 0;
        node->balance = 0;
        node->child[0] = NULL;
        node->child[1] = NULL;
    }

    /* build the tree, the index array is the permutation of the nodes
     * kd_build() is recursive, its depth is the depth of the tree */
    b.coords = t->pool;
    b.uids = uids;
    for (i = 0; i < m; i++)
        idx[i] = i;

    t->root = kd_build(t, &b, idx, 0, m, 0);
    t->count = m;

    G_free(idx);

    return t;
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
        kd_free_node(t, s[top].n);
        s[top].n = NULL;
        t->count--;
        if (top) {
            top--;
            n = s[top].n;
            dir = s[top].dir;
            n->child[dir] = NULL;

            /* update node */
            kd_update_node(t, n);
        }
        else {
            t->root = NULL;

            return 1;
        }
    }
    else {
        /* replace the node with a point of its subtree, kd_replace() is
         * called by the recursive kd_balance() */
        kd_replace(t, s[top].n);
    }

    while (top) {
        top--;
        n = s[top].n;

        /* update node */
        kd_update_node(t, n);
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
             * slower but more compact
             * kd_balance() is recursive, it calls kd_insert2() again to
             * insert the replaced item */
            if (!bmode2) {
                while (kd_balance(t, n, bmode))
                    ;
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
                kd_update_node(t, n);
                if (bmode2) {
                    while (kd_balance(t, n, bmode))
                        ;
                }
                top--;
                if (top >= 0) {
                    kd_update_node(t, s[top].n);
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
 * level 0 = a bit, 1 = more, 2 = a lot
 * the balancing below is recursive, kd_balance() calls kd_insert2() which
 * calls kd_balance() again
 */
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

    if (t->nodes) {
        /* the tree was created by kdtree_create_from_points() and is already
         * perfectly balanced */
        G_debug(1, "k-d tree is already balanced");

        return;
    }

    G_debug(1, "k-d tree optimization for %zd items, tree depth %d", t->count,
            t->root->depth);

    nbal = 0;
    top = 0;
    s[top].n = t->root;
    while (s[top].n) {
        n = s[top].n;

        ld = (!n->child[0] ? -1 : n->child[0]->depth);
        rd = (!n->child[1] ? -1 : n->child[1]->depth);

        if (ld < rd)
            while (kd_balance(t, n->child[0], level))
                ;
        else if (ld > rd)
            while (kd_balance(t, n->child[1], level))
                ;

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
        while (kd_balance(t, n, level)) {
            nbal++;
        }
        while (kd_balance(t, n->child[0], level))
            ;
        while (kd_balance(t, n->child[1], level))
            ;

        ld = (!n->child[0] ? -1 : n->child[0]->depth);
        rd = (!n->child[1] ? -1 : n->child[1]->depth);
        n->depth = MAX(ld, rd) + 1;

        while (kd_balance(t, n, level)) {
            nbal++;
        }
    }

    while (s[top].n) {
        n = s[top].n;

        /* balance node */
        while (kd_balance(t, n, level)) {
            nbal++;
        }
        while (kd_balance(t, n->child[0], level))
            ;
        while (kd_balance(t, n->child[1], level))
            ;

        ld = (!n->child[0] ? -1 : n->child[0]->depth);
        rd = (!n->child[1] ? -1 : n->child[1]->depth);
        n->depth = MAX(ld, rd) + 1;

        while (kd_balance(t, n, level)) {
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
            while (kd_balance(t, n, level)) {
                nbal++;
            }
            while (kd_balance(t, n->child[0], level))
                ;
            while (kd_balance(t, n->child[1], level))
                ;

            ld = (!n->child[0] ? -1 : n->child[0]->depth);
            rd = (!n->child[1] ? -1 : n->child[1]->depth);
            n->depth = MAX(ld, rd) + 1;

            while (kd_balance(t, n, level)) {
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

    G_debug(1, "k-d tree optimization: %d times balanced, new depth %d", nbal,
            t->root->depth);

    return;
}

/* find k nearest neighbors
 * results are stored in uid (uids) and d (squared distances)
 * optionally an uid to be skipped can be given
 * useful when searching for the nearest neighbors of an item
 * that is also in the tree */
int kdtree_knn(struct kdtree *t, double *c, int *uid, double *d, int k,
               int *skip)
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

    maxdist = INFINITY;
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
        return kd_first(trav, c, uid);
    }

    return kd_next(trav, c, uid);
}

/**********************************************/
/*            internal functions              */

/**********************************************/

static struct kdnode *kd_newnode(struct kdtree *t)
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

static void kd_free_node(struct kdtree *t, struct kdnode *n)
{
    /* the nodes of a tree created by kdtree_create_from_points() are stored
     * in one contiguous array which is freed when the tree is cleared, so
     * these nodes must not be freed here
     * the node is recognized by its address in that array, so the array has
     * to stay one allocation of t->nnodes nodes */
    if (t->nodes) {
        uintptr_t first = (uintptr_t)t->nodes;
        uintptr_t last = first + t->nnodes * sizeof(struct kdnode);
        uintptr_t addr = (uintptr_t)n;

        if (addr >= first && addr < last)
            return;
    }

    G_free(n->c);
    G_free(n);
}

static void kd_update_node(struct kdtree *t, struct kdnode *n)
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

/* check if two points are identical, i.e. they have the same coordinates
 * and, if duplicate coordinates are not allowed, the same uid */
static int kd_same_point(const struct kdbuild *b, int i, int j)
{
    if (cmp_point_coords(b, i, j))
        return 0;
    if (b->dc && b->uids[i] != b->uids[j])
        return 0;

    return 1;
}

static void kd_swap(int *a, int *b)
{
    int tmp = *a;

    *a = *b;
    *b = tmp;
}

/* sort the point indices in the range [lo, hi)
 * recursive: the range is split at the pivot and the smaller part is sorted
 * recursively, so the recursion depth is limited by the logarithm of the
 * range size
 */
static void kd_sort(struct kdbuild *b, int *idx, int lo, int hi, int mode,
                    int dim)
{
    int i;

    while (hi - lo > 16) {
        int mid = lo + (hi - lo) / 2;
        int last = hi - 1;
        int pivot, lt, gt;

        /* median of three */
        if (cmp_points(b, idx[mid], idx[lo], mode, dim) < 0)
            kd_swap(&idx[mid], &idx[lo]);
        if (cmp_points(b, idx[last], idx[mid], mode, dim) < 0) {
            kd_swap(&idx[last], &idx[mid]);
            if (cmp_points(b, idx[mid], idx[lo], mode, dim) < 0)
                kd_swap(&idx[mid], &idx[lo]);
        }
        pivot = idx[mid];

        /* partition the range into the points which are smaller than the
         * pivot, the points which are equal to the pivot and the points which
         * are larger than the pivot */
        lt = lo;
        i = lo;
        gt = hi;
        while (i < gt) {
            int c = cmp_points(b, idx[i], pivot, mode, dim);

            if (c < 0)
                kd_swap(&idx[lt++], &idx[i++]);
            else if (c > 0)
                kd_swap(&idx[i], &idx[--gt]);
            else
                i++;
        }

        /* continue with the larger part and recurse into the smaller one to
         * limit the recursion depth */
        if (lt - lo < hi - gt) {
            kd_sort(b, idx, lo, lt, mode, dim);
            lo = gt;
        }
        else {
            kd_sort(b, idx, gt, hi, mode, dim);
            hi = lt;
        }
    }

    /* insertion sort for small ranges */
    for (i = lo + 1; i < hi; i++) {
        int val = idx[i];
        int j = i - 1;

        while (j >= lo && cmp_points(b, val, idx[j], mode, dim) < 0) {
            idx[j + 1] = idx[j];
            j--;
        }
        idx[j + 1] = val;
    }
}

/* find the position of the point with the rank k (starting from 0) in the
 * range [lo, hi)
 * not recursive itself, but it can fall back to kd_sort() which is recursive
 */
static int kd_select(struct kdbuild *b, int *idx, int lo, int hi, int k,
                     int dim)
{
    int bad = 0;

    while (hi - lo > 1) {
        int mid = lo + (hi - lo) / 2;
        int last = hi - 1;
        int pivot, i, j, pos, oldsize;

        /* median of three, the pivot ends up at the position mid */
        if (cmp_points(b, idx[mid], idx[lo], KD_CMP_DIM, dim) < 0)
            kd_swap(&idx[mid], &idx[lo]);
        if (cmp_points(b, idx[last], idx[mid], KD_CMP_DIM, dim) < 0) {
            kd_swap(&idx[last], &idx[mid]);
            if (cmp_points(b, idx[mid], idx[lo], KD_CMP_DIM, dim) < 0)
                kd_swap(&idx[mid], &idx[lo]);
        }
        pivot = idx[mid];

        /* partition the range, the pivot is moved to the end of the range
         * and then to its final position */
        kd_swap(&idx[mid], &idx[last]);
        i = lo;
        for (j = lo; j < last; j++) {
            if (cmp_points(b, idx[j], pivot, KD_CMP_DIM, dim) < 0) {
                kd_swap(&idx[i], &idx[j]);
                i++;
            }
        }
        kd_swap(&idx[i], &idx[last]);
        pos = i;

        /* the pivot is at its final position */
        oldsize = hi - lo;
        if (k < pos - lo)
            hi = pos;
        else if (k > pos - lo) {
            k -= pos + 1 - lo;
            lo = pos + 1;
        }
        else
            return pos;

        /* sort the remaining range if the partition is too unbalanced
         * kd_sort() is recursive */
        if (oldsize - (hi - lo) < oldsize / 8)
            bad++;
        else
            bad = 0;
        if (bad > 3) {
            kd_sort(b, idx, lo, hi, KD_CMP_DIM, dim);

            return lo + k;
        }
    }

    return lo;
}

/* build the tree from the points in the range [lo, hi) of the index array
 * the points are split at their median
 * recursive: the two halves of the range are built recursively, so the
 * recursion depth is the depth of the tree
 */
static struct kdnode *kd_build(struct kdtree *t, struct kdbuild *b, int *idx,
                               int lo, int hi, int dim)
{
    struct kdnode *node;
    int n = hi - lo;
    int pivot, mid, lt, i, gt, ld, rd;

    if (n <= 0)
        return NULL;

    if (n == 1) {
        node = &t->nodes[idx[lo]];
        node->dim = (unsigned char)dim;
        node->depth = 0;
        node->balance = 0;
        node->child[0] = NULL;
        node->child[1] = NULL;

        return node;
    }

    /* find the median of the points */
    mid = kd_select(b, idx, lo, hi, (n - 1) / 2, dim);
    pivot = idx[mid];

    /* points which compare equal to the pivot must not be separated by the
     * split because the tree search uses the same order to find a point
     * kd_select() orders the range by (c[dim], uid, index), so the points
     * which compare equal to the pivot differ only in the index and they form
     * a block around the pivot, checking its two neighbours is enough to
     * detect that block */
    if ((mid > lo && !cmp_point_order(b, idx[mid - 1], pivot, dim)) ||
        (mid + 1 < hi && !cmp_point_order(b, idx[mid + 1], pivot, dim))) {
        /* partition the points into the points which are smaller than the
         * pivot, the points which are equal to the pivot and the points which
         * are larger than the pivot */
        lt = lo;
        i = lo;
        gt = hi;
        while (i < gt) {
            int c = cmp_point_order(b, idx[i], pivot, dim);

            if (c < 0)
                kd_swap(&idx[lt++], &idx[i++]);
            else if (c > 0)
                kd_swap(&idx[i], &idx[--gt]);
            else
                i++;
        }

        /* the pivot is the last point of the group of equal points */
        for (i = lt; i < gt; i++) {
            if (idx[i] == pivot) {
                kd_swap(&idx[i], &idx[gt - 1]);
                break;
            }
        }
        mid = gt - 1;
    }

    node = &t->nodes[pivot];
    node->dim = (unsigned char)dim;
    node->balance = 0;
    /* build the two halves of the range, kd_build() is recursive */
    node->child[0] = kd_build(t, b, idx, lo, mid, t->nextdim[dim]);
    node->child[1] = kd_build(t, b, idx, mid + 1, hi, t->nextdim[dim]);

    ld = node->child[0] ? node->child[0]->depth : -1;
    rd = node->child[1] ? node->child[1]->depth : -1;
    node->depth = MAX(ld, rd) + 1;

    return node;
}

/* find a replacement for the node r and replace r with it
 * not recursive itself, but it is called by kd_balance() which is recursive
 */
static int kd_replace(struct kdtree *t, struct kdnode *r)
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

    /* do not call kd_balance in this fn, this can cause
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
                    G_message("rn c %g, or child c %g", rn->c[i],
                              or->child[1]->c[i]);
                G_fatal_error(
                    "Right child of old root is smaller than rn, dir is %d",
                    ordir);
            }
        }
        if (or->child[0]) {
            dir = cmp(or->child[0], rn, or->dim);
            if (dir > 0) {
                int i;

                for (i = 0; i < t->ndims; i++)
                    G_message("rn c %g, or child c %g", rn->c[i],
                              or->child[0]->c[i]);
                G_fatal_error(
                    "Left child of old root is larger than rn, dir is %d",
                    ordir);
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
    kd_free_node(t, rn);
    n->child[dir] = NULL;
    t->count--;

    kd_update_node(t, n);
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
        kd_update_node(t, n);
    }

    return nr;
}

/* balance the (sub-)tree of the node r
 * recursive: kd_balance() calls kd_insert2() to insert the replaced item and
 * kd_insert2() calls kd_balance() again, so the recursion depth grows with
 * the depth of the tree
 */
static int kd_balance(struct kdtree *t, struct kdnode *r, int bmode)
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
        kd_update_node(t, r);
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

    or = kd_newnode(t);
    memcpy(or->c, r->c, t->csize);
    or->uid = r->uid;
    or->dim = t->nextdim[r->dim];

    if (!kd_replace(t, r))
        G_fatal_error("kd_balance: nothing replaced");

#ifdef KD_DEBUG
    if (!cmp(r, or, r->dim)) {
        G_warning("kd_balance: replacement failed");
        kd_free_node(t, or);

        return 0;
    }
#endif

    /* insert the replaced item again, kd_insert2() is recursive */
    r->child[!dir] = kd_insert2(t, r->child[!dir], or, bmode, 1); /* bmode */

    /* update node */
    kd_update_node(t, r);

    if (r->depth == old_depth) {
        G_debug(4, "balancing had no effect");
        return 1;
    }

    if (r->depth > old_depth)
        G_fatal_error("balancing failed");

    return 1;
}

/* insert an item into the (sub-)tree
 * recursive: the balancing calls kd_balance() which calls kd_insert2() again
 * to insert the replaced item, so the recursion depth grows with the depth of
 * the tree
 */
static struct kdnode *kd_insert2(struct kdtree *t, struct kdnode *r,
                                 struct kdnode *nnew, int balance, int dc)
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
            kd_free_node(t, nnew);

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
        kd_update_node(t, n);

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
             * slower but more compact
             * kd_balance() is recursive, it calls kd_insert2() again to
             * insert the replaced item */
            if (!bmode2) {
                while (kd_balance(t, n, bmode))
                    ;
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
                 * faster but less compact
                 * kd_balance() is recursive, it calls kd_insert2() again to
                 * insert the replaced item */
                if (bmode2) {
                    while (kd_balance(t, n, bmode))
                        ;
                }
                top--;
                if (top >= 0) {
                    kd_update_node(t, s[top].n);
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
static int kd_first(struct kdtrav *trav, double *c, int *uid)
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
static int kd_next(struct kdtrav *trav, double *c, int *uid)
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

    return 0; /* finished traversing */
}
