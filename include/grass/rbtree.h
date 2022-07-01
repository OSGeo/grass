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
 * struct RB_TREE *mytree = rbtree_create(my_compare_fn, item_size);
 *
 * insert items to tree:
 * struct mydatastruct data = <some data>;
 * if (rbtree_insert(mytree, &data) == 0)
 * 	 G_warning("could not insert data");
 *
 * find item in tree:
 * struct mydatastruct data = <some data>;
 * if (rbtree_find(mytree, &data) == 0)
 * 	 G_message("data not found");
 *
 * delete item from tree:
 * struct mydatastruct data = <some data>;
 * if (rbtree_remove(mytree, &data) == 0)
 * 	  G_warning("could not find data in tree");
 *
 * traverse tree (get all items in tree in ascending order):
 * struct RB_TRAV trav;
 * rbtree_init_trav(&trav, tree);
 * while ((data = rbtree_traverse(&trav)) != NULL) {
 *   if (my_compare_fn(data, threshold_data) == 0) break;
 * 	   <do something with data>;
 *  }
 *
 * get a selection of items: all data > data1 and < data2
 * start in tree where data is last smaller or first larger compared to data1
 * struct RB_TRAV trav;
 * rbtree_init_trav(&trav, tree);
 * data = rbtree_traverse_start(&trav, &data1);
 * 	 <do something with data>;
 * while ((data = rbtree_traverse(&trav)) != NULL) {
 *	 if (data > data2) break;
 *   <do something with data>;
 * }
 *
 * destroy tree:
 * rbtree_destroy(mytree);
 *
 * debug the whole tree with
 * rbtree_debug(mytree, mytree->root);
 * 
 *************************************************************/

#ifndef GRASS_RBTREE_H
#define GRASS_RBTREE_H

#include <stddef.h>

/* maximum RB Tree height */
#define RBTREE_MAX_HEIGHT 64        /* should be more than enough */

/* routine to compare data items
 * return -1 if rb_a < rb_b
 * return  0 if rb_a == rb_b
 * return  1 if rb_a > rb_b
 */
typedef int rb_compare_fn(const void *rb_a, const void *rb_b);

struct RB_NODE
{
    unsigned char red;              /* 0 = black, 1 = red */
    void *data;                     /* any kind of data */
    struct RB_NODE *link[2];        /* link to children: link[0] for smaller, link[1] for larger */
};
 
struct RB_TREE
{
    struct RB_NODE *root;           /* root node */
    size_t datasize;                /* item size */
    size_t count;                   /* number of items in tree. */
    rb_compare_fn *rb_compare;      /* function to compare data */
};

struct RB_TRAV
{
    struct RB_TREE *tree;           /* tree being traversed */
    struct RB_NODE *curr_node;      /* current node */
    struct RB_NODE *up[RBTREE_MAX_HEIGHT];  /* stack of parent nodes */
    int top;                        /* index for stack */
    int first;                      /* little helper flag */
};

#include <grass/defs/rbtree.h>

#endif
