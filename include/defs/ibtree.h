#ifndef GRASS_IBTREEDEFS_H
#define GRASS_IBTREEDEFS_H

int ibtree_create(IBTREE *, int (*)(), int);
int ibtree_find(IBTREE *, int, int *);
int ibtree_free(IBTREE *);
int ibtree_next(IBTREE *, int *, int *);
int ibtree_rewind(IBTREE *);
int Btree_init();
int Btree_add(int);
int Btree_report();
int ibtree_update(IBTREE *, int, int);

#endif
