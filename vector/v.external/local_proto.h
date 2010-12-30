#ifndef V_EXTERNAL_LOCAL_PROTO_H
#define V_EXTERNAL_LOCAL_PROTO_H

struct _options {
    struct Option *dsn, *output, *layer;
};

struct _flags {
    struct Flag *format, *layer, *topo;
};

/* args.c */
void parse_args(int, char **,
		struct _options *, struct _flags*);
void get_args(const struct _options *, const struct _flags *,
	      char **, char **, char **,
	      int *, int *, int *);

/* list.c */
void list_formats(FILE *);
int list_layers(FILE *, const char *, const char *, int *);

#endif
