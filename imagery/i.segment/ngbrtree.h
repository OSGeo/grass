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
 * struct NB_TREE *mytree = nbtree_create(my_compare_fn, item_size);
 *
 * insert items to tree:
 * struct mydatastruct data = <some data>;
 * if (nbtree_insert(mytree, &data) == 0)
 * 	 G_warning("could not insert data");
 *
 * find item in tree:
 * struct mydatastruct data = <some data>;
 * if (nbtree_find(mytree, &data) == 0)
 * 	 G_message("data not found");
 *
 * delete item from tree:
 * struct mydatastruct data = <some data>;
 * if (nbtree_remove(mytree, &data) == 0)
 * 	  G_warning("could not find data in tree");
 *
 * traverse tree (get all items in tree in ascending order):
 * struct NB_TRAV trav;
 * nbtree_init_trav(&trav, tree);
 * while ((data = nbtree_traverse(&trav)) != NULL) {
 *   if (my_compare_fn(data, threshold_data) == 0) break;
 * 	   <do something with data>;
 *  }
 *
 * get a selection of items: all data > data1 and < data2
 * start in tree where data is last smaller or first larger compared to data1
 * struct NB_TRAV trav;
 * nbtree_init_trav(&trav, tree);
 * data = nbtree_traverse_start(&trav, &data1);
 * 	 <do something with data>;
 * while ((data = nbtree_traverse(&trav)) != NULL) {
 *	 if (data > data2) break;
 *   <do something with data>;
 * }
 *
 * destroy tree:
 * nbtree_destroy(mytree);
 *
 * debug the whole tree with
 * nbtree_debug(mytree, mytree->root);
 * 
 *************************************************************/

#ifndef NBTREE_H
#define NBTREE_H

#include <stddef.h>

/* maximum RB Tree height */
#define NBTREE_MAX_HEIGHT 64        /* should be more than enough */

/* routine to compare data items
 * return -1 if rb_a < rb_b
 * return  0 if rb_a == rb_b
 * return  1 if rb_a > rb_b
 */
struct ngbr_stats
{
    int id;		/* region ID */
    int row, col;	/* row, col of one cell in this region */
    int count;		/* number cells in this region */
    double *mean;	/* mean for each band = sum[b] / count */
};


struct NB_NODE
{
    unsigned char red;              /* 0 = black, 1 = red */
    struct NB_NODE *link[2];        /* link to children: link[0] for smaller, link[1] for larger */
    struct ngbr_stats data;           /* any kind of data */
};
 
struct NB_TREE
{
    struct NB_NODE *root;           /* root node */
    size_t datasize;                /* item size */
    size_t count;                   /* number of items in tree. */
    int nbands;			    /* number of bands */
};

struct NB_TRAV
{
    struct NB_TREE *tree;           /* tree being traversed */
    struct NB_NODE *curr_node;      /* current node */
    struct NB_NODE *up[NBTREE_MAX_HEIGHT];  /* stack of parent nodes */
    int top;                        /* index for stack */
    int first;                      /* little helper flag */
};


/* tree functions */
struct NB_TREE *nbtree_create(int, size_t);
void nbtree_clear(struct NB_TREE *);
int nbtree_insert(struct NB_TREE *, struct ngbr_stats *);
int nbtree_remove(struct NB_TREE *, struct ngbr_stats *);
struct ngbr_stats *nbtree_find(struct NB_TREE *, struct ngbr_stats *);

/* tree traversal functions */
int nbtree_init_trav(struct NB_TRAV *, struct NB_TREE *);
struct ngbr_stats *nbtree_traverse(struct NB_TRAV *);
struct ngbr_stats *nbtree_traverse_start(struct NB_TRAV *, struct ngbr_stats *);

/* debug tree from given node downwards */
int nbtree_debug(struct NB_TREE *, struct NB_NODE *);

#endif
