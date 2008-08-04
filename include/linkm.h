#ifndef GRASS_LINKM_H
#define GRASS_LINKM_H

#ifndef FILE
#  include <stdio.h>
#endif

#define VOID_T char

#define PTR_CNT 10

struct link_head
{
    VOID_T **ptr_array;		/* array of pointers to chunks */
    int max_ptr;		/* num of chunks alloced */
    int alloced;		/* size of ptr_array */
    int chunk_size;		/* size of alloc chucks in units */
    int unit_size;		/* size of each user defined unit */
    VOID_T *Unused;		/* Unused list pointer */
    int exit_flag;		/* exit on error ? */
};

#endif

/* destroy.c */
void link_destroy(struct link_head *, VOID_T *);

/* dispose.c */
void link_dispose(struct link_head *, VOID_T *);

/* init.c */
void link_set_chunk_size(int);
void link_exit_on_error(int);
struct link_head *link_init(int);
void link_cleanup(struct link_head *);

/* new.c */
struct link_head *link_new(struct link_head *);

/* next.c */
VOID_T *link__get_next(VOID_T *);
void link__set_next(VOID_T *, VOID_T *);

/* oom.c */
int link_out_of_memory(void);
