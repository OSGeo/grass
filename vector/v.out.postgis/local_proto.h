#ifndef __LOCAL_PROTO_V_OUT_POSTGIS__
#define __LOCAL_PROTO_V_OUT_POSTGIS__

#include <grass/vector.h>

struct params {
    struct Option *input, *layer, *dsn, *olayer, *opts, *olink;
};

struct flags {
    struct Flag *table, *topo;
};

/* create.c */
void create_table(struct Map_info *, struct Map_info *);
char *create_pgfile(const char *, const char *, const char *, char **, int,
		    char **, char **);

/* options.c */
void define_options(struct params *, struct flags *);

/* table.c */
void check_columns(const struct Map_info *, const char *, const char *, const char *);

#endif /* __LOCAL_PROTO_V_OUT_POSTGIS__ */
