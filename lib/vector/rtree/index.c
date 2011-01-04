
/****************************************************************************
* MODULE:       R-Tree library 
*              
* AUTHOR(S):    Antonin Guttman - original code
*               Daniel Green (green@superliminal.com) - major clean-up
*                               and implementation of bounding spheres
*               Markus Metz - file-based and memory-based R*-tree
*               
* PURPOSE:      Multidimensional index
*
* COPYRIGHT:    (C) 2010 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*****************************************************************************/

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <grass/gis.h>
#include "index.h"
#include "card.h"

/* 
 * Make a new index, empty.
 * fp pointer to file holding index, file must be opened as w+
 * rootpos postion of rootnode (past any header info)
 * ndims number of dimensions
 * returns pointer to RTree structure
 */
struct RTree *RTreeNewIndex(int fd, off_t rootpos, int ndims)
{
    struct RTree *new_rtree;
    struct Node *n;
    int i;
    
    new_rtree = (struct RTree *)malloc(sizeof(struct RTree));

    new_rtree->fd = fd;
    new_rtree->rootpos = rootpos;
    new_rtree->ndims = ndims;
    new_rtree->nsides = 2 * ndims;
    
    new_rtree->rectsize = sizeof(struct Rect);

    /* init free nodes */
    new_rtree->free_nodes.avail = 0;
    new_rtree->free_nodes.alloc = 0;
    new_rtree->free_nodes.pos = NULL;

    new_rtree->nodesize = sizeof(struct Node);
    new_rtree->branchsize = sizeof(struct Branch);
    
    /* create empty root node */
    n = RTreeNewNode(new_rtree, 0);
    new_rtree->rootlevel = n->level = 0;       /* leaf */
    new_rtree->root = NULL;

    if (fd > -1) {  /* file based */
	/* nodecard and leafcard can be adjusted, must NOT be larger than MAXCARD */
	new_rtree->nodecard = MAXCARD;
	new_rtree->leafcard = MAXCARD;

	/* initialize node buffer */
	for (i = 0; i < MAXLEVEL; i++) {
	    new_rtree->nb[i][0].dirty = 0;
	    new_rtree->nb[i][1].dirty = 0;
	    new_rtree->nb[i][2].dirty = 0;
	    new_rtree->nb[i][0].pos = -1;
	    new_rtree->nb[i][1].pos = -1;
	    new_rtree->nb[i][2].pos = -1;
	    /* usage order */
	    new_rtree->used[i][0] = 2;
	    new_rtree->used[i][1] = 1;
	    new_rtree->used[i][2] = 0;
	}

	/* write empty root node */
	lseek(new_rtree->fd, rootpos, SEEK_SET);
	RTreeWriteNode(n, new_rtree);
	new_rtree->nb[0][0].n = *n;
	new_rtree->nb[0][0].pos = rootpos;
	new_rtree->used[0][0] = 0;
	new_rtree->used[0][2] = 2;
	RTreeFreeNode(n);

	new_rtree->insert_rect = RTreeInsertRectF;
	new_rtree->delete_rect = RTreeDeleteRectF;
	new_rtree->search_rect = RTreeSearchF;
	new_rtree->valid_child = RTreeValidChildF;
	
    }
    else {    /* memory based */
	new_rtree->nodecard = MAXCARD;
	new_rtree->leafcard = MAXCARD;

	new_rtree->insert_rect = RTreeInsertRectM;
	new_rtree->delete_rect = RTreeDeleteRectM;
	new_rtree->search_rect = RTreeSearchM;
	new_rtree->valid_child = RTreeValidChildM;

	new_rtree->root = n;
    }

    /* minimum number of remaining children for RTreeDeleteRect */
    /* NOTE: min fill can be changed if needed, must be < nodecard and leafcard. */
    new_rtree->min_node_fill = (new_rtree->nodecard - 2) / 2;
    new_rtree->min_leaf_fill = (new_rtree->leafcard - 2) / 2;

    /* balance criteria for node splitting */
    new_rtree->minfill_node_split = (new_rtree->nodecard - 1) / 2;
    new_rtree->minfill_leaf_split = (new_rtree->leafcard - 1) / 2;

    new_rtree->n_nodes = 1;
    new_rtree->n_leafs = 0;

    return new_rtree;
}

void RTreeFreeIndex(struct RTree *t)
{
    assert(t);

    if (t->fd > -1) {
	if (t->free_nodes.alloc)
	    free(t->free_nodes.pos);
    }
    else if (t->root)
	RTreeDestroyNode(t->root, t->root->level ? t->nodecard : t->leafcard);

    free(t);
}

/*
 * Search in an index tree for all data retangles that
 * overlap the argument rectangle.
 * Return the number of qualifying data rects.
 */
int RTreeSearch(struct RTree *t, struct Rect *r, SearchHitCallback *shcb,
		void *cbarg)
{
    assert(r && t);

    return t->search_rect(t, r, shcb, cbarg);
}

/* 
 * Insert a data rectangle into an RTree index structure.
 * r pointer to rectangle
 * tid data id stored with rectangle, must be > 0
 * t RTree where rectangle should be inserted
 */
int RTreeInsertRect(struct Rect *r, int tid, struct RTree *t)
{
    union Child newchild;

    assert(r && t && tid > 0);
    
    t->n_leafs++;
    newchild.id = tid;
    
    return t->insert_rect(r, newchild, 0, t);
}

/* 
 * Delete a data rectangle from an index structure.
 * Pass in a pointer to a Rect, the tid of the record, ptr RTree.
 * Returns 1 if record not found, 0 if success.
 * RTreeDeleteRect1 provides for eliminating the root.
 *
 * RTreeDeleteRect() should be called by external functions instead of 
 * RTreeDeleteRect1()
 * wrapper for RTreeDeleteRect1 not really needed, but restricts 
 * compile warnings to rtree lib
 * this way it's easier to fix if necessary? 
 */
int RTreeDeleteRect(struct Rect *r, int tid, struct RTree *t)
{
    union Child child;
    
    assert(r && t && tid > 0);

    child.id = tid;

    return t->delete_rect(r, child, t);
}

/*
 * Allocate space for a node in the list used in DeleteRect to
 * store Nodes that are too empty.
 */
struct ListNode *RTreeNewListNode(void)
{
    return (struct ListNode *)malloc(sizeof(struct ListNode));
}

void RTreeFreeListNode(struct ListNode *p)
{
    free(p);
}

/* 
 * Add a node to the reinsertion list.  All its branches will later
 * be reinserted into the index structure.
 */
void RTreeReInsertNode(struct Node *n, struct ListNode **ee)
{
    struct ListNode *l = RTreeNewListNode();

    l->node = n;
    l->next = *ee;
    *ee = l;
}

/* 
 * Free ListBranch
 */
void RTreeFreeListBranch(struct ListBranch *p)
{
    free(p);
}



