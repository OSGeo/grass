
/****************************************************************************
* MODULE:       R-Tree library 
*              
* AUTHOR(S):    Antonin Guttman - original code
*               Daniel Green (green@superliminal.com) - major clean-up
*                               and implementation of bounding spheres
*               Markus Metz - R*-tree
*               
* PURPOSE:      Multidimensional index
*
* COPYRIGHT:    (C) 2009 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*****************************************************************************/
#ifndef _INDEX_
#define _INDEX_

#include <sys/types.h>

/* PGSIZE is normally the natural page size of the machine */
#define PGSIZE	512
#define NUMDIMS	3		/* maximum number of dimensions */

/* typedef float RectReal; */
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
#define FORCECARD 2

/* maximum no of levels = tree depth */
#define MAXLEVEL 20        /* 4^MAXLEVEL items are guaranteed to fit into the tree */

struct Rect
{
    RectReal boundary[NUMSIDES];	/* xmin,ymin,...,xmax,ymax,... */
};

struct Node;               /* node for memory based index */

union Child
{
    int id;              /* child id */
    struct Node *ptr;    /* pointer to child node */
};

struct Branch              /* branch for memory based index */
{
    struct Rect rect;
    union Child child;
};

struct Node             /* node for memory based index */
{
    int count;          /* number of branches */
    int level;			/* 0 is leaf, others positive */
    struct Branch branch[MAXCARD];
};

struct RTree
{
    /* RTree setup info */
    unsigned char ndims;    /* number of dimensions */
    unsigned char nsides;   /* number of sides = 2 * ndims */
    int nodesize;           /* node size in bytes */
    int branchsize;         /* branch size in bytes */
    int rectsize;           /* rectangle size in bytes */

    /* RTree info, useful to calculate space requirements */
    unsigned int n_nodes;   /* number of nodes */
    unsigned int n_leafs;   /* number of data items (level 0 leafs) */
    int n_levels;           /* n levels = root level */
    
    /* settings for RTree building */
    int nodecard;           /* max number of childs in node */
    int leafcard;           /* max number of childs in leaf */
    int min_node_fill;      /* balance criteria for node splitting */
    int min_leaf_fill;      /* balance criteria for leaf splitting */
    
    struct Node *root;    /* pointer to root node */

    off_t rootpos;         /* root node position in file */
};

struct ListNode
{
    struct ListNode *next;
    struct Node *node;
};

struct ListBranch
{
    struct ListBranch *next;
    struct Branch b;
    int level;
};

/*
 * If passed to a tree search, this callback function will be called
 * with the ID of each data rect that overlaps the search rect
 * plus whatever user specific pointer was passed to the search.
 * It can terminate the search early by returning 0 in which case
 * the search will return the number of hits found up to that point.
 */
typedef int (*SearchHitCallback) (int id, void *arg);

/* index.c */
extern int RTreeSearch(struct RTree *, struct Rect *, SearchHitCallback,
		       void *);
extern int RTreeInsertRect(struct Rect *, int, struct RTree *t);
extern int RTreeDeleteRect(struct Rect *, int, struct RTree *t);
extern struct RTree *RTreeNewIndex(int);
void RTreeFreeIndex(struct RTree *);
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
extern int RTreeOverlap(struct Rect *, struct Rect *, struct RTree *);
extern void RTreePrintRect(struct Rect *, int);
/* split.c */
extern void RTreeSplitNode(struct Node *, struct Branch *, struct Node *, struct RTree *);
/* card.c */
extern int RTreeSetNodeMax(int, struct RTree *);
extern int RTreeSetLeafMax(int, struct RTree *);
extern int RTreeGetNodeMax(struct RTree *);
extern int RTreeGetLeafMax(struct RTree *);

#endif /* _INDEX_ */
