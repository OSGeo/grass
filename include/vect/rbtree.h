/*************************************************************
 *                          USAGE
 *************************************************************
 *
 * NOTE: duplicates are not supported
 *
 * custom compare function
 * extern int my_compare_fn(const void *, const void *);
 * int my_compare_fn(const void *a, const void *b) {
 *    if ((mydatastruct *) a > (mydatastruct *) b)
 *      return 0;
 *    else if ((mydatastruct *) a < (mydatastruct *) b)
 *      return 1;
 *    else if ((mydatastruct *) a == (mydatastruct *) b)
 *      return 2;
 * }
 * 
 * create and initialize tree:
 * struct RB_TREE *mytree = rbtree_create(my_compare_fn, item_size);
 *
 * insert items to tree:
 * struct mydatastruct data = <some data>;
 * if (rbtree_insert(&mytree, &data) == 0)
 * 	G_warning("could not insert data");
 *
 * find item in tree:
 * struct mydatastruct data = <some data>;
 * if (rbtree_find(&mytree, &data) == 0)
 * 	G_message("data not found");
 *
 * delete item from tree:
 * struct mydatastruct data = <some data>;
 * if (rbtree_remove(&mytree, &data) == 0)
 * 	G_warning("could not find data in tree");
 *
 * traverse tree (get all items in tree in ascending order):
 * struct RB_TRAV trav;
 * rbtree_init_trav(&trav, tree);
 * while ((data = rbtree_traverse(&trav)) != NULL) {
 *	if (my_compare_fn(data, threshold_data) == 0) break;
 * 	<do something with data>;
 * }
 *
 * destroy tree:
 * rbtree_destroy(mytree);
 * 
 *************************************************************/



/* structures and functions needed for Red Black Tree */

#include <stddef.h>

/* maximum RB Tree height */
#define RBTREE_MAX_HEIGHT 64 /* allow 2^64 items */

/* routine to compare data items */
/* return 0 if rb_a > rb_b */
/* return 1 if rb_a < rb_b */
/* return 2 if rb_a == rb_b */
typedef int rb_compare_fn(const void *rb_a, const void *rb_b);

struct RB_NODE
{
    unsigned char red;
    void *data;
    struct RB_NODE *link[2];
};
 
struct RB_TREE
{
    struct RB_NODE *root;
    size_t datasize;                /* item size */
    size_t count;                   /* Number of items in tree. */
    rb_compare_fn *rb_compare;
};

struct RB_TRAV
{
    struct RB_TREE *tree;       /* tree being traversed */
    struct RB_NODE *curr_node;
    struct RB_NODE *up[RBTREE_MAX_HEIGHT];
    int top;
    int first;
};

/* rbtree.c */
struct RB_TREE *rbtree_create(rb_compare_fn *, size_t);
int rbtree_insert(struct RB_TREE *, void *);
void *rbtree_find(struct RB_TREE *, const void *);
int rbtree_remove(struct RB_TREE *, const void *);
void rbtree_init_trav(struct RB_TRAV *, struct RB_TREE *);
void* rbtree_traverse(struct RB_TRAV *);
void rbtree_destroy(struct RB_TREE *);
int rbtree_debug(struct RB_TREE *, struct RB_NODE *);
