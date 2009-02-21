
/****************************************************************************
* MODULE:       R-Tree library 
*              
* AUTHOR(S):    Antonin Guttman - original code
*               Daniel Green (green@superliminal.com) - major clean-up
*                               and implementation of bounding spheres
*               
* PURPOSE:      Multidimensional index
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*****************************************************************************/
#ifndef _INDEX_
#define _INDEX_

/* PGSIZE is normally the natural page size of the machine */
#define PGSIZE	512
#define NUMDIMS	3		/* number of dimensions */

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

struct Rect
{
    RectReal boundary[NUMSIDES];	/* xmin,ymin,...,xmax,ymax,... */
};

struct Node;

struct Branch
{
    struct Rect rect;
    struct Node *child;
};

/* max branching factor of a node */
#define MAXCARD (int)((PGSIZE-(2*sizeof(int))) / sizeof(struct Branch))

struct Node
{
    int count;
    int level;			/* 0 is leaf, others positive */
    struct Branch branch[MAXCARD];
};

struct ListNode
{
    struct ListNode *next;
    struct Node *node;
};

/*
 * If passed to a tree search, this callback function will be called
 * with the ID of each data rect that overlaps the search rect
 * plus whatever user specific pointer was passed to the search.
 * It can terminate the search early by returning 0 in which case
 * the search will return the number of hits found up to that point.
 */
typedef int (*SearchHitCallback) (int id, void *arg);


extern int RTreeSearch(struct Node *, struct Rect *, SearchHitCallback,
		       void *);
extern int RTreeInsertRect(struct Rect *, int, struct Node **, int depth);
extern int RTreeInsertRect1(struct Rect *, struct Node *, struct Node **, int depth);
extern int RTreeDeleteRect(struct Rect *, int, struct Node **);
extern int RTreeDeleteRect1(struct Rect *, struct Node *, struct Node **);
extern struct Node *RTreeNewIndex(void);
extern struct Node *RTreeNewNode(void);
extern void RTreeInitNode(struct Node *);
extern void RTreeFreeNode(struct Node *);
extern void RTreeDestroyNode(struct Node *);
extern void RTreePrintNode(struct Node *, int);
extern void RTreeTabIn(int);
extern struct Rect RTreeNodeCover(struct Node *);
extern void RTreeInitRect(struct Rect *);
extern struct Rect RTreeNullRect(void);
extern RectReal RTreeRectArea(struct Rect *);
extern RectReal RTreeRectSphericalVolume(struct Rect *R);
extern RectReal RTreeRectVolume(struct Rect *R);
extern struct Rect RTreeCombineRect(struct Rect *, struct Rect *);
extern int RTreeOverlap(struct Rect *, struct Rect *);
extern void RTreePrintRect(struct Rect *, int);
extern int RTreeAddBranch(struct Branch *, struct Node *, struct Node **);
extern int RTreePickBranch(struct Rect *, struct Node *);
extern void RTreeDisconnectBranch(struct Node *, int);
extern void RTreeSplitNode(struct Node *, struct Branch *, struct Node **);

extern int RTreeSetNodeMax(int);
extern int RTreeSetLeafMax(int);
extern int RTreeGetNodeMax(void);
extern int RTreeGetLeafMax(void);

#endif /* _INDEX_ */
