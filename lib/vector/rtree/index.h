
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
#ifndef _INDEX_
#define _INDEX_

#include <stdio.h>
#include <sys/types.h>
#include <grass/config.h>

/* PGSIZE is normally the natural page size of the machine */
#define PGSIZE	512
#define NUMDIMS	3		/* maximum number of dimensions */

typedef double RectReal;

/*-----------------------------------------------------------------------------
| Global definitions.
-----------------------------------------------------------------------------*/

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define NUMSIDES 2*NUMDIMS

/* max branching factor of a node */
/* was (PGSIZE-(2 * sizeof(int))) / sizeof(struct Branch)
 * this is LFS dependent, not good
 * on 32 bit without LFS this is 9.69
 * on 32 bit with LFS and on 64 bit this is 9 */
#define MAXCARD 9

/* R*-tree: number of branches to be force-reinserted when adding a branch */
#define FORCECARD 3

/* maximum no of levels = tree depth */
#define MAXLEVEL 20        /* 8^MAXLEVEL items are guaranteed to fit into the tree */

#define NODETYPE(l, fd) ((l) == 0 ? 0 : ((fd) < 0 ? 1 : 2))

struct RTree_Rect
{
    RectReal boundary[NUMSIDES];	/* xmin,ymin,...,xmax,ymax,... */
};

struct RTree_Node;               /* node for spatial index */

union RTree_Child
{
    int id;              /* child id */
    struct RTree_Node *ptr;    /* pointer to child node */
    off_t pos;           /* position of child node in file */
};

struct RTree_Branch              /* branch for spatial index */
{
    struct RTree_Rect rect;
    union RTree_Child child;
};

struct RTree_Node             /* node for spatial index */
{
    int count;          /* number of branches */
    int level;		/* 0 is leaf, others positive */
    struct RTree_Branch branch[MAXCARD];
};

/*
 * If passed to a tree search, this callback function will be called
 * with the ID of each data rect that overlaps the search rect
 * plus whatever user specific pointer was passed to the search.
 * It can terminate the search early by returning 0 in which case
 * the search will return the number of hits found up to that point.
 */
typedef int SearchHitCallback(int id, struct RTree_Rect rect, void *arg);

struct RTree;

typedef int rt_search_fn(struct RTree *, struct RTree_Rect *,
                         SearchHitCallback *, void *);
typedef int rt_insert_fn(struct RTree_Rect *, union RTree_Child, int, struct RTree *);
typedef int rt_delete_fn(struct RTree_Rect *, union RTree_Child, struct RTree *);
typedef int rt_valid_child_fn(union RTree_Child *);

struct RTree
{
    /* RTree setup info */
    int fd;                 /* file descriptor */
    unsigned char ndims;    /* number of dimensions */
    unsigned char nsides;   /* number of sides = 2 * ndims */
    int nodesize;           /* node size in bytes */
    int branchsize;         /* branch size in bytes */
    int rectsize;           /* rectangle size in bytes */

    /* RTree info, useful to calculate space requirements */
    int n_nodes;            /* number of nodes */
    int n_leafs;            /* number of data items (level 0 leafs) */
    int rootlevel;          /* root level = tree depth */
    
    /* settings for RTree building */
    int nodecard;           /* max number of childs in node */
    int leafcard;           /* max number of childs in leaf */
    int min_node_fill;      /* balance criteria for node removal */
    int min_leaf_fill;      /* balance criteria for leaf removal */
    int minfill_node_split; /* balance criteria for splitting */
    int minfill_leaf_split; /* balance criteria for splitting */
    
    /* free node positions for recycling */
    struct _recycle {
        int avail;          /* number of available positions */
        int alloc;          /* number of allcoated positions in *pos */
        off_t *pos;         /* array of available positions */
    } free_nodes;

    /* node buffer for file-based index, three nodes per level
     * more than three nodes per level would require too complex cache management */
    struct NodeBuffer
    {
	struct RTree_Node n;	    /* buffered node */
	off_t pos;	    /* file position of buffered node */
	char dirty;         /* node in buffer was modified */
    } nb[MAXLEVEL][3];

    /* usage order of buffered nodes per level
     * used[level][0] = most recently used
     * used[level][2] = least recently used */
    char used[MAXLEVEL][3];

    /* insert, delete, search */
    rt_insert_fn *insert_rect;
    rt_delete_fn *delete_rect;
    rt_search_fn *search_rect;
    rt_valid_child_fn *valid_child;
    
    struct RTree_Node *root;     /* pointer to root node */

    off_t rootpos;         /* root node position in file */
};

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

/* index.c */
extern int RTreeSearch(struct RTree *, struct RTree_Rect *, SearchHitCallback *,
		       void *);
extern int RTreeInsertRect(struct RTree_Rect *, int, struct RTree *);
extern int RTreeDeleteRect(struct RTree_Rect *, int, struct RTree *);
extern struct RTree *RTreeNewIndex(int, off_t, int);
extern void RTreeFreeIndex(struct RTree *);
extern struct RTree_ListNode *RTreeNewListNode(void);
extern void RTreeFreeListNode(struct RTree_ListNode *);
extern void RTreeReInsertNode(struct RTree_Node *, struct RTree_ListNode **);
extern void RTreeFreeListBranch(struct RTree_ListBranch *);
/* indexm.c */
extern int RTreeSearchM(struct RTree *, struct RTree_Rect *, SearchHitCallback *,
		       void *);
extern int RTreeInsertRectM(struct RTree_Rect *, union RTree_Child, int, struct RTree *);
extern int RTreeDeleteRectM(struct RTree_Rect *, union RTree_Child, struct RTree *);
extern int RTreeValidChildM(union RTree_Child *child);
/* indexf.c */
extern int RTreeSearchF(struct RTree *, struct RTree_Rect *, SearchHitCallback *,
		       void *);
extern int RTreeInsertRectF(struct RTree_Rect *, union RTree_Child, int, struct RTree *);
extern int RTreeDeleteRectF(struct RTree_Rect *, union RTree_Child, struct RTree *);
extern int RTreeValidChildF(union RTree_Child *child);
/* node.c */
extern struct RTree_Node *RTreeNewNode(struct RTree *, int);
extern void RTreeInitNode(struct RTree_Node *, int);
extern void RTreeFreeNode(struct RTree_Node *);
extern void RTreeDestroyNode(struct RTree_Node *, int);
extern struct RTree_Rect RTreeNodeCover(struct RTree_Node *, struct RTree *);
extern int RTreeAddBranch(struct RTree_Branch *, struct RTree_Node *, struct RTree_Node **, 
            struct RTree_ListBranch **, struct RTree_Rect *, int *, struct RTree *);
extern int RTreePickBranch(struct RTree_Rect *, struct RTree_Node *, struct RTree *);
extern void RTreeDisconnectBranch(struct RTree_Node *, int, struct RTree *);
extern void RTreePrintNode(struct RTree_Node *, int, struct RTree *);
extern void RTreeTabIn(int);
/* rect.c */
extern void RTreeInitRect(struct RTree_Rect *);
extern struct RTree_Rect RTreeNullRect(void);
extern RectReal RTreeRectArea(struct RTree_Rect *, struct RTree *);
extern RectReal RTreeRectSphericalVolume(struct RTree_Rect *, struct RTree *);
extern RectReal RTreeRectVolume(struct RTree_Rect *, struct RTree *);
extern RectReal RTreeRectMargin(struct RTree_Rect *, struct RTree *);
extern struct RTree_Rect RTreeCombineRect(struct RTree_Rect *, struct RTree_Rect *, struct RTree *);
extern int RTreeCompareRect(struct RTree_Rect *, struct RTree_Rect *, struct RTree *);
extern int RTreeOverlap(struct RTree_Rect *, struct RTree_Rect *, struct RTree *);
extern void RTreePrintRect(struct RTree_Rect *, int);
/* split.c */
extern void RTreeSplitNode(struct RTree_Node *, struct RTree_Branch *, struct RTree_Node *, struct RTree *);
/* card.c */
extern int RTreeSetNodeMax(int, struct RTree *);
extern int RTreeSetLeafMax(int, struct RTree *);
extern int RTreeGetNodeMax(struct RTree *);
extern int RTreeGetLeafMax(struct RTree *);
/* fileio.c */
extern void RTreeGetNode(struct RTree_Node *, off_t, int, struct RTree *);
extern size_t RTreeReadNode(struct RTree_Node *, off_t, struct RTree *);
extern void RTreePutNode(struct RTree_Node *, off_t, struct RTree *);
extern size_t RTreeWriteNode(struct RTree_Node *, struct RTree *);
extern size_t RTreeRewriteNode(struct RTree_Node *, off_t, struct RTree *);
extern void RTreeUpdateRect(struct RTree_Rect *, struct RTree_Node *, off_t, int, struct RTree *);
extern void RTreeAddNodePos(off_t, int, struct RTree *);
extern off_t RTreeGetNodePos(struct RTree *);
extern void RTreeFlushBuffer(struct RTree *);

#endif /* _INDEX_ */
