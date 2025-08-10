#ifndef GRASS_BTREEDEFS_H
#define GRASS_BTREEDEFS_H

/* create.c */
int btree_create(BTREE *, int (*)(const void *, const void *), int);

/* find.c */
int btree_find(const BTREE *, const void *, void **);

/* free.c */
int btree_free(BTREE *);

/* next.c */
int btree_next(BTREE *, void **, void **);

/* rewind.c */
int btree_rewind(BTREE *);

/* update.c */
int btree_update(BTREE *, const void *, int, const void *, int);

#endif
