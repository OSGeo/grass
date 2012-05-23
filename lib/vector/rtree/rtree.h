
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
* COPYRIGHT:    (C) 2010-2012 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*****************************************************************************/
#ifndef _R_TREE_H_
#define _R_TREE_H_

#include <stdio.h>
#include <sys/types.h>
#include <grass/config.h> /* needed for LFS */


#define NUMDIMS	3		/* maximum number of dimensions */
#define NUMSIDES 2*NUMDIMS

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

/* max branching factor of a node */
/* was (PGSIZE-(2 * sizeof(int))) / sizeof(struct Branch)
 * this is LFS dependent, not good
 * on 32 bit without LFS this is 9.69
 * on 32 bit with LFS and on 64 bit this is 9 */
#define MAXCARD 9
#define NODECARD MAXCARD
#define LEAFCARD MAXCARD


/* maximum no of levels = tree depth */
#define MAXLEVEL 20        /* 8^MAXLEVEL items are guaranteed to fit into the tree */

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
    int fd;                 	  /* file descriptor */
    unsigned char ndims;    	  /* number of dimensions */
    unsigned char nsides;   	  /* number of sides = 2 * ndims */
    unsigned char nsides_alloc;   /* number of sides = 2 * ndims */
    int nodesize;           	  /* node size in bytes */
    int branchsize;         	  /* branch size in bytes */
    int rectsize;           	  /* rectangle size in bytes */

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

/* RTree main functions */
int RTreeSearch(struct RTree *, struct RTree_Rect *, SearchHitCallback *,
		       void *);
int RTreeInsertRect(struct RTree_Rect *, int, struct RTree *);
int RTreeDeleteRect(struct RTree_Rect *, int, struct RTree *);
struct RTree *RTreeNewIndex(int, off_t, int);
void RTreeFreeIndex(struct RTree *);
int RTreeOverlap(struct RTree_Rect *, struct RTree_Rect *, struct RTree *);

/* RTree node management */
struct RTree_Node *RTreeNewNode(struct RTree *, int);
void RTreeInitNode(struct RTree *, struct RTree_Node *, int);
void RTreeFreeNode(struct RTree_Node *);
void RTreeDestroyNode(struct RTree_Node *, int);

/* RTree IO */
size_t RTreeReadNode(struct RTree_Node *, off_t, struct RTree *);
size_t RTreeWriteNode(struct RTree_Node *, struct RTree *);
off_t RTreeGetNodePos(struct RTree *);
void RTreeFlushBuffer(struct RTree *);

#endif
