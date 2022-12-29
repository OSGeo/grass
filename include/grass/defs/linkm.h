#ifndef GRASS_LINKMDEFS_H
#define GRASS_LINKMDEFS_H

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
VOID_T *link_new(struct link_head *);


/* for internal use only */
/* next.c */
VOID_T *link__get_next(VOID_T *);
void link__set_next(VOID_T *, VOID_T *);

/* oom.c */
int link_out_of_memory(void);

#endif
