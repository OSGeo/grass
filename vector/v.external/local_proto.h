#ifndef V_EXTERNAL_LOCAL_PROTO_H
#define V_EXTERNAL_LOCAL_PROTO_H

struct _options {
    struct Option *dsn, *output, *layer;
};

struct _flags {
    struct Flag *format, *list, *tlist, *topo;
};

/* args.c */
void parse_args(int, char **,
		struct _options *, struct _flags*);

/* list.c */
void list_formats(FILE *);
int list_layers(FILE *, const char *, const char *, int, int *);

#endif
