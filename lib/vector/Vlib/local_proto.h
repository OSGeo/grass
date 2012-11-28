#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/vector.h>

void V2__add_line_to_topo_nat(struct Map_info *, int ,
                              const struct line_pnts *, const struct line_cats *,
                              int (*external_routine) (const struct Map_info *, int));

#endif /* PG_LOCAL_PROTO_H__ */
