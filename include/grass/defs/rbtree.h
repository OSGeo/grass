#ifndef GRASS_RBTREEDEFS_H
#define GRASS_RBTREEDEFS_H

/* tree functions */
struct RB_TREE *rbtree_create(rb_compare_fn *, size_t);
void rbtree_clear(struct RB_TREE *);
void rbtree_destroy(struct RB_TREE *);
int rbtree_insert(struct RB_TREE *, void *);
int rbtree_remove(struct RB_TREE *, const void *);
void *rbtree_find(struct RB_TREE *, const void *);

/* tree traversal functions */
int rbtree_init_trav(struct RB_TRAV *, struct RB_TREE *);
void* rbtree_traverse(struct RB_TRAV *);
void *rbtree_traverse_backwd(struct RB_TRAV *trav);
void *rbtree_traverse_start(struct RB_TRAV *, const void *);

/* debug tree from given node downwards */
int rbtree_debug(struct RB_TREE *, struct RB_NODE *);

#endif
