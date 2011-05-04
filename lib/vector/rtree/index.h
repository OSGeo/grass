
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
#define MAXCARD 18

/* R*-tree: number of branches to be force-reinserted when adding a branch */
#define FORCECARD 2

/* maximum no of levels = tree depth */
#define MAXLEVEL 20        /* 8^MAXLEVEL items are guaranteed to fit into the tree */

#define NODETYPE(l, fd) ((l) == 0 ? 0 : ((fd) < 0 ? 1 : 2))

struct Rect
{
    RectReal boundary[NUMSIDES];	/* xmin,ymin,...,xmax,ymax,... */
};

struct Node;               /* node for spatial index */

union Child
{
    int id;              /* child id */
    struct Node *ptr;    /* pointer to child node */
    off_t pos;           /* position of child node in file */
};

struct Branch              /* branch for spatial index */
{
    struct Rect rect;
    union Child child;
};

struct Node             /* node for spatial index */
{
    int count;          /* number of branches */
    int level;		/* 0 is leaf, others positive */
    struct Branch branch[MAXCARD];
};

/*
 * If passed to a tree search, this callback function will be called
 * with the ID of each data rect that overlaps the search rect
 * plus whatever user specific pointer was passed to the search.
 * It can terminate the search early by returning 0 in which case
 * the search will return the number of hits found up to that point.
 */
typedef int SearchHitCallback(int id, void *arg);

struct RTree;

typedef int rt_search_fn(struct RTree *, struct Rect *,
                         SearchHitCallback *, void *);
typedef int rt_insert_fn(struct Rect *, union Child, int, struct RTree *);
typedef int rt_delete_fn(struct Rect *, union Child, struct RTree *);
typedef int rt_valid_child_fn(union Child *);

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
	struct Node n;	    /* buffered node */
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
    
    struct Node *root;     /* pointer to root node */

    off_t rootpos;         /* root node position in file */
};

struct ListNode
{
    struct ListNode *next;
    struct Node *node;
};

struct ListFNode
{
    struct ListFNode *next;
    off_t node_pos;
};

struct ListBranch
{
    struct ListBranch *next;
    struct Branch b;
    int level;
};

/* index.c */
extern int RTreeSearch(struct RTree *, struct Rect *, SearchHitCallback *,
		       void *);
extern int RTreeInsertRect(struct Rect *, int, struct RTree *);
extern int RTreeDeleteRect(struct Rect *, int, struct RTree *);
extern struct RTree *RTreeNewIndex(int, off_t, int);
extern void RTreeFreeIndex(struct RTree *);
extern struct ListNode *RTreeNewListNode(void);
extern void RTreeFreeListNode(struct ListNode *);
extern void RTreeReInsertNode(struct Node *, struct ListNode **);
extern void RTreeFreeListBranch(struct ListBranch *);
/* indexm.c */
extern int RTreeSearchM(struct RTree *, struct Rect *, SearchHitCallback *,
		       void *);
extern int RTreeInsertRectM(struct Rect *, union Child, int, struct RTree *);
extern int RTreeDeleteRectM(struct Rect *, union Child, struct RTree *);
extern int RTreeValidChildM(union Child *child);
/* indexf.c */
extern int RTreeSearchF(struct RTree *, struct Rect *, SearchHitCallback *,
		       void *);
extern int RTreeInsertRectF(struct Rect *, union Child, int, struct RTree *);
extern int RTreeDeleteRectF(struct Rect *, union Child, struct RTree *);
extern int RTreeValidChildF(union Child *child);
/* node.c */
extern struct Node *RTreeNewNode(struct RTree *, int);
extern void RTreeInitNode(struct Node *, int);
extern void RTreeFreeNode(struct Node *);
extern void RTreeDestroyNode(struct Node *, int);
extern struct Rect RTreeNodeCover(struct Node *, struct RTree *);
extern int RTreeAddBranch(struct Branch *, struct Node *, struct Node **, 
            struct ListBranch **, struct Rect *, int *, struct RTree *);
extern int RTreePickBranch(struct Rect *, struct Node *, struct RTree *);
extern void RTreeDisconnectBranch(struct Node *, int, struct RTree *);
extern void RTreePrintNode(struct Node *, int, struct RTree *);
extern void RTreeTabIn(int);
/* rect.c */
extern void RTreeInitRect(struct Rect *);
extern struct Rect RTreeNullRect(void);
extern RectReal RTreeRectArea(struct Rect *, struct RTree *);
extern RectReal RTreeRectSphericalVolume(struct Rect *, struct RTree *);
extern RectReal RTreeRectVolume(struct Rect *, struct RTree *);
extern RectReal RTreeRectMargin(struct Rect *, struct RTree *);
extern struct Rect RTreeCombineRect(struct Rect *, struct Rect *, struct RTree *);
extern int RTreeCompareRect(struct Rect *, struct Rect *, struct RTree *);
extern int RTreeOverlap(struct Rect *, struct Rect *, struct RTree *);
extern void RTreePrintRect(struct Rect *, int);
/* split.c */
extern void RTreeSplitNode(struct Node *, struct Branch *, struct Node *, struct RTree *);
/* card.c */
extern int RTreeSetNodeMax(int, struct RTree *);
extern int RTreeSetLeafMax(int, struct RTree *);
extern int RTreeGetNodeMax(struct RTree *);
extern int RTreeGetLeafMax(struct RTree *);
/* fileio.c */
extern void RTreeGetNode(struct Node *, off_t, int, struct RTree *);
extern size_t RTreeReadNode(struct Node *, off_t, struct RTree *);
extern void RTreePutNode(struct Node *, off_t, struct RTree *);
extern size_t RTreeWriteNode(struct Node *, struct RTree *);
extern size_t RTreeRewriteNode(struct Node *, off_t, struct RTree *);
extern void RTreeUpdateRect(struct Rect *, struct Node *, off_t, int, struct RTree *);
extern void RTreeAddNodePos(off_t, int, struct RTree *);
extern off_t RTreeGetNodePos(struct RTree *);
extern void RTreeFlushBuffer(struct RTree *);

#endif /* _INDEX_ */
