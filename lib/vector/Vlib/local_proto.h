#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/vector.h>

void V2__add_line_to_topo_nat(struct Map_info *, int ,
                              const struct line_pnts *, const struct line_cats *,
                              int (*external_routine) (const struct Map_info *, int));

/* map.c */
int Vect__delete(const char *, int);

/* open.c */
int Vect__open_old(struct Map_info *, const char *, const char *,
                   const char *, int, int, int);
char *Vect__get_path(const struct Map_info *);
char *Vect__get_element_path(const struct Map_info *, const char *);

#endif /* PG_LOCAL_PROTO_H__ */
