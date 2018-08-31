
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
#ifndef _R_TREE_INDEX_H_
#define _R_TREE_INDEX_H_

#include "rtree.h"

/* internal definitions and functions */

/* PGSIZE is normally the natural page size of the machine */
#define PGSIZE	512

/* R*-tree: number of branches to be force-reinserted when adding a branch */
#define FORCECARD 3

#define NODETYPE(l, fd) ((l) == 0 ? 0 : ((fd) < 0 ? 1 : 2))


struct RTree_ListNode
{
    struct RTree_ListNode *next;
    struct RTree_Node *node;
};

struct RTree_ListFNode
{
    struct RTree_ListFNode *next;
    off_t node_pos;
};

struct RTree_ListBranch
{
    struct RTree_ListBranch *next;
    struct RTree_Branch b;
    int level;
};

/* functions */

/* index.c */
struct RTree_ListNode *RTreeNewListNode(void);
void RTreeFreeListNode(struct RTree_ListNode *);
void RTreeReInsertNode(struct RTree_Node *, struct RTree_ListNode **);
void RTreeFreeListBranch(struct RTree_ListBranch *);

/* indexm.c */
int RTreeSearchM(struct RTree *, struct RTree_Rect *,
                 SearchHitCallback *, void *);
int RTreeInsertRectM(struct RTree_Rect *, union RTree_Child, int, struct RTree *);
int RTreeDeleteRectM(struct RTree_Rect *, union RTree_Child, struct RTree *);
int RTreeValidChildM(union RTree_Child *child);

/* indexf.c */
int RTreeSearchF(struct RTree *, struct RTree_Rect *,
                 SearchHitCallback *, void *);
int RTreeInsertRectF(struct RTree_Rect *, union RTree_Child, int, struct RTree *);
int RTreeDeleteRectF(struct RTree_Rect *, union RTree_Child, struct RTree *);
int RTreeValidChildF(union RTree_Child *);

/* node.c */
void RTreeNodeCover(struct RTree_Node *, struct RTree_Rect *, struct RTree *);
int RTreeAddBranch(struct RTree_Branch *, struct RTree_Node *, struct RTree_Node **, 
            struct RTree_ListBranch **, struct RTree_Rect *, char *, struct RTree *);
int RTreePickBranch(struct RTree_Rect *, struct RTree_Node *, struct RTree *);
void RTreeDisconnectBranch(struct RTree_Node *, int, struct RTree *);
void RTreePrintNode(struct RTree_Node *, int, struct RTree *);
void RTreeTabIn(int);
void RTreeCopyBranch(struct RTree_Branch *, struct RTree_Branch *, struct RTree *);

/* rect.c */
void RTreeInitRect(struct RTree_Rect *, struct RTree *);
void RTreeNullRect(struct RTree_Rect *, struct RTree *);
RectReal RTreeRectArea(struct RTree_Rect *, struct RTree *);
RectReal RTreeRectSphericalVolume(struct RTree_Rect *, struct RTree *);
RectReal RTreeRectVolume(struct RTree_Rect *, struct RTree *);
RectReal RTreeRectMargin(struct RTree_Rect *, struct RTree *);
void RTreeCombineRect(struct RTree_Rect *, struct RTree_Rect *, struct RTree_Rect *, struct RTree *);
int RTreeExpandRect(struct RTree_Rect *, struct RTree_Rect *, struct RTree *);
int RTreeCompareRect(struct RTree_Rect *, struct RTree_Rect *, struct RTree *);

/*-----------------------------------------------------------------------------
| Copy second rectangle to first rectangle.
-----------------------------------------------------------------------------*/
#define RTreeCopyRect(r1, r2, t) memcpy((r1)->boundary, (r2)->boundary, (t)->rectsize)


/* split.c */
void RTreeSplitNode(struct RTree_Node *, struct RTree_Branch *, struct RTree_Node *, struct RTree *);

/* card.c */
int RTreeSetNodeMax(int, struct RTree *);
int RTreeSetLeafMax(int, struct RTree *);
int RTreeGetNodeMax(struct RTree *);
int RTreeGetLeafMax(struct RTree *);

/* io.c */
struct RTree_Node *RTreeGetNode(off_t, int, struct RTree *);
void RTreeNodeChanged(struct RTree_Node *, off_t , struct RTree *);
size_t RTreeRewriteNode(struct RTree_Node *, off_t, struct RTree *);
void RTreeAddNodePos(off_t, int, struct RTree *);

#endif /* _INDEX_ */
