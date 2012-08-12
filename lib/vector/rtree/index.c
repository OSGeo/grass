
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
//#include "card.h"

/* 
 * Make a new index, empty.
 * fp pointer to file holding index, file must be opened as w+
 * rootpos postion of rootnode (past any header info)
 * ndims number of dimensions
 * returns pointer to RTree structure
 */
struct RTree *RTreeCreateTree(int fd, off_t rootpos, int ndims)
{
    struct RTree *new_rtree;
    struct RTree_Node *n;
    int i, j;
    
    new_rtree = (struct RTree *)malloc(sizeof(struct RTree));

    new_rtree->fd = fd;
    new_rtree->rootpos = rootpos;
    new_rtree->ndims = ndims;
    new_rtree->nsides = 2 * ndims;
    /* hack to keep compatibility */
    if (ndims < 3)
	new_rtree->ndims_alloc = 3;
    else
	new_rtree->ndims_alloc = ndims;

    new_rtree->nsides_alloc = 2 * new_rtree->ndims_alloc;

    /* init free nodes */
    new_rtree->free_nodes.avail = 0;
    new_rtree->free_nodes.alloc = 0;
    new_rtree->free_nodes.pos = NULL;

    new_rtree->rectsize = new_rtree->nsides_alloc * sizeof(RectReal);
    new_rtree->nodesize = sizeof(struct RTree_Node) -
                          MAXCARD * sizeof(RectReal *) +
			  MAXCARD * new_rtree->rectsize;

    new_rtree->branchsize = sizeof(struct RTree_Branch) -
                            sizeof(RectReal *) + new_rtree->rectsize;
    
    /* create empty root node */
    n = RTreeAllocNode(new_rtree, 0);
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

	    /* alloc memory for rectangles */
	    for (j = 0; j < MAXCARD; j++) {
		RTreeAllocBoundary(&(new_rtree->nb[i][0].n.branch[j].rect), new_rtree);
		RTreeAllocBoundary(&(new_rtree->nb[i][1].n.branch[j].rect), new_rtree);
		RTreeAllocBoundary(&(new_rtree->nb[i][2].n.branch[j].rect), new_rtree);

		RTreeAllocBoundary(&(new_rtree->fs[i].sn.branch[j].rect), new_rtree);
	    }
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

    /* initialize temp variables */
    RTreeAllocBoundary(&(new_rtree->p.cover[0]), new_rtree);
    RTreeAllocBoundary(&(new_rtree->p.cover[1]), new_rtree);
    
    RTreeAllocBoundary(&(new_rtree->tmpb1.rect), new_rtree);
    RTreeAllocBoundary(&(new_rtree->tmpb2.rect), new_rtree);
    RTreeAllocBoundary(&(new_rtree->c.rect), new_rtree);
    for (i = 0; i <= MAXCARD; i++) {
	RTreeAllocBoundary(&(new_rtree->BranchBuf[i].rect), new_rtree);
    }
    RTreeAllocBoundary(&(new_rtree->rect_0), new_rtree);
    RTreeAllocBoundary(&(new_rtree->rect_1), new_rtree);
    RTreeAllocBoundary(&(new_rtree->upperrect), new_rtree);
    RTreeAllocBoundary(&(new_rtree->orect), new_rtree);
    new_rtree->center_n = (RectReal *)malloc(new_rtree->ndims_alloc * sizeof(RectReal));

    return new_rtree;
}

void RTreeDestroyTree(struct RTree *t)
{
    int i, j;

    assert(t);

    if (t->fd > -1) {
	if (t->free_nodes.alloc)
	    free(t->free_nodes.pos);
    }
    else if (t->root)
	RTreeDestroyNode(t->root, t->root->level ? t->nodecard : t->leafcard);

    if (t->fd > -1) {  /* file based */
	/* free node buffer */
	for (i = 0; i < MAXLEVEL; i++) {

	    /* free memory for rectangles */
	    for (j = 0; j < MAXCARD; j++) {
		RTreeFreeBoundary(&(t->nb[i][0].n.branch[j].rect));
		RTreeFreeBoundary(&(t->nb[i][1].n.branch[j].rect));
		RTreeFreeBoundary(&(t->nb[i][2].n.branch[j].rect));
		RTreeFreeBoundary(&(t->fs[i].sn.branch[j].rect));
	    }
	}
    }

    /* free temp variables */
    RTreeFreeBoundary(&(t->p.cover[0]));
    RTreeFreeBoundary(&(t->p.cover[1]));
    
    RTreeFreeBoundary(&(t->tmpb1.rect));
    RTreeFreeBoundary(&(t->tmpb2.rect));
    RTreeFreeBoundary(&(t->c.rect));
    for (i = 0; i <= MAXCARD; i++) {
        RTreeFreeBoundary(&(t->BranchBuf[i].rect));
    }
    RTreeFreeBoundary(&(t->rect_0));
    RTreeFreeBoundary(&(t->rect_1));
    RTreeFreeBoundary(&(t->upperrect));
    RTreeFreeBoundary(&(t->orect));
    free(t->center_n);

    free(t);
    
    return;
}

/*
 * Search in an index tree for all data retangles that
 * overlap or touch the argument rectangle.
 * Return the number of qualifying data rects.
 * 
 * add option to select operator to select rectangles ?
 * current: overlap
 * possible alternatives: 
 *  - select all rectangles that are fully contained in r
 *  - select all rectangles that fully contain r
 */
int RTreeSearch(struct RTree *t, struct RTree_Rect *r,
                SearchHitCallback *shcb, void *cbarg)
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
int RTreeInsertRect(struct RTree_Rect *r, int tid, struct RTree *t)
{
    union RTree_Child newchild;

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
int RTreeDeleteRect(struct RTree_Rect *r, int tid, struct RTree *t)
{
    union RTree_Child child;
    
    assert(r && t && tid > 0);

    child.id = tid;

    return t->delete_rect(r, child, t);
}

/*
 * Allocate space for a node in the list used in DeleteRect to
 * store Nodes that are too empty.
 */
struct RTree_ListNode *RTreeNewListNode(void)
{
    return (struct RTree_ListNode *)malloc(sizeof(struct RTree_ListNode));
}

void RTreeFreeListNode(struct RTree_ListNode *p)
{
    free(p);
}

/* 
 * Add a node to the reinsertion list.  All its branches will later
 * be reinserted into the index structure.
 */
void RTreeReInsertNode(struct RTree_Node *n, struct RTree_ListNode **ee)
{
    struct RTree_ListNode *l = RTreeNewListNode();

    l->node = n;
    l->next = *ee;
    *ee = l;
}

/* 
 * Free ListBranch
 */
void RTreeFreeListBranch(struct RTree_ListBranch *p)
{
    RTreeFreeBoundary(&(p->b.rect));
    free(p);
}



