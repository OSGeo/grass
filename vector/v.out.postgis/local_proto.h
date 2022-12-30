#ifndef __LOCAL_PROTO_V_OUT_POSTGIS__
#define __LOCAL_PROTO_V_OUT_POSTGIS__

#include <grass/vector.h>

struct params {
    struct Option *input, *layer, *dsn, *olayer, *opts, *olink, *type;
};

struct flags {
    struct Flag *table, *topo, *force2d;
};

/* args.c */
void define_options(struct params *, struct flags *);

/* create.c */
char *create_pgfile(const char *, const char *, const char *, char **, int,
                    char **, char **);

/* table.c */
<<<<<<< HEAD
<<<<<<< HEAD
void check_columns(struct Map_info *, const char *, const char *, const char *);
=======
void check_columns(const struct Map_info *, const char *, const char *,
                   const char *);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void check_columns(const struct Map_info *, const char *, const char *,
                   const char *);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

#endif /* __LOCAL_PROTO_V_OUT_POSTGIS__ */
