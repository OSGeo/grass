#ifndef __LOCAL_PROTO_V_OUT_POSTGIS__
#define __LOCAL_PROTO_V_OUT_POSTGIS__

#include <grass/vector.h>

struct params {
    struct Option *input, *layer, *dsn, *olayer;
};

struct flags {
    struct Flag *table, *topo;
};

/* export.c */
int export_lines(struct Map_info *, int, struct Map_info *);
int export_areas(struct Map_info *, int, struct Map_info *);

/* export_topo.c */
int export_topo(struct Map_info *, int, struct Map_info *);

/* options.c */
void define_options(struct params *, struct flags *);
char *create_pgfile(const char *, int);

#endif /* __LOCAL_PROTO_V_OUT_POSTGIS__ */
