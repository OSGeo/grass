/*************************************************************
 *                          USAGE                            *
 *************************************************************
 *
 * NOTE: duplicates are not supported
 *
 * custom compare function
 * extern int my_compare_fn(const void *, const void *);
 * int my_compare_fn(const void *a, const void *b) {
 *   if ((mydatastruct *) a < (mydatastruct *) b)
 *     return -1;
 *   else if ((mydatastruct *) a > (mydatastruct *) b)
 *     return 1;
 *   else if ((mydatastruct *) a == (mydatastruct *) b)
 *     return 0;
 * }
 * 
 * create and initialize tree:
 * struct RG_TREE *mytree = rgtree_create(my_compare_fn, item_size);
 *
 * insert items to tree:
 * struct mydatastruct data = <some data>;
 * if (rgtree_insert(mytree, &data) == 0)
 * 	 G_warning("could not insert data");
 *
 * find item in tree:
 * struct mydatastruct data = <some data>;
 * if (rgtree_find(mytree, &data) == 0)
 * 	 G_message("data not found");
 *
 * delete item from tree:
 * struct mydatastruct data = <some data>;
 * if (rgtree_remove(mytree, &data) == 0)
 * 	  G_warning("could not find data in tree");
 *
 * traverse tree (get all items in tree in ascending order):
 * struct RG_TRAV trav;
 * rgtree_init_trav(&trav, tree);
 * while ((data = rgtree_traverse(&trav)) != NULL) {
 *   if (my_compare_fn(data, threshold_data) == 0) break;
 * 	   <do something with data>;
 *  }
 *
 * get a selection of items: all data > data1 and < data2
 * start in tree where data is last smaller or first larger compared to data1
 * struct RG_TRAV trav;
 * rgtree_init_trav(&trav, tree);
 * data = rgtree_traverse_start(&trav, &data1);
 * 	 <do something with data>;
 * while ((data = rgtree_traverse(&trav)) != NULL) {
 *	 if (data > data2) break;
 *   <do something with data>;
 * }
 *
 * destroy tree:
 * rgtree_destroy(mytree);
 *
 * debug the whole tree with
 * rgtree_debug(mytree, mytree->root);
 * 
 *************************************************************/

#ifndef REGTREE_H
#define REGTREE_H

#include <stddef.h>

/* maximum RB Tree height */
#define REGTREE_MAX_HEIGHT 64        /* should be more than enough */

/* routine to compare data items
 * return -1 if rb_a < rb_b
 * return  0 if rb_a == rb_b
 * return  1 if rb_a > rb_b
 */
struct reg_stats
{
    int id;		/* region ID */
    int count;		/* number of cells of this region */
    double *sum,	/* sum for each band */
           *mean;	/* mean for each band = sum[b] / count */

	   /* unused; stddev and thus sumsq may be needed for 
	    * eCognition-like multi-scale segmentation */
	   /*
	   *min,
	   *max,
	   *sumsq,
	   *stddev;
	   */
};

typedef int rg_compare_fn(struct reg_stats *rb_a, struct reg_stats *rb_b);

struct RG_NODE
{
    unsigned char red;              /* 0 = black, 1 = red */
    struct RG_NODE *link[2];        /* link to children: link[0] for smaller, link[1] for larger */
    struct reg_stats data;           /* any kind of data */
};
 
struct RG_TREE
{
    struct RG_NODE *root;           /* root node */
    size_t datasize;                /* item size */
    size_t count;                   /* number of items in tree. */
    int nbands;			    /* number of bands */
    rg_compare_fn *cmp;      /* function to compare data */
};

struct RG_TRAV
{
    struct RG_TREE *tree;           /* tree being traversed */
    struct RG_NODE *curr_node;      /* current node */
    struct RG_NODE *up[REGTREE_MAX_HEIGHT];  /* stack of parent nodes */
    int top;                        /* index for stack */
    int first;                      /* little helper flag */
};


/* tree functions */
struct RG_TREE *rgtree_create(int, size_t);
void rgtree_destroy(struct RG_TREE *);
int rgtree_insert(struct RG_TREE *, struct reg_stats *);
int rgtree_remove(struct RG_TREE *, struct reg_stats *);
struct reg_stats *rgtree_find(struct RG_TREE *, struct reg_stats *);

/* tree traversal functions */
int rgtree_init_trav(struct RG_TRAV *, struct RG_TREE *);
struct reg_stats *rgtree_traverse(struct RG_TRAV *);
struct reg_stats *rgtree_traverse_start(struct RG_TRAV *, struct reg_stats *);

/* debug tree from given node downwards */
int rgtree_debug(struct RG_TREE *, struct RG_NODE *);

#endif
