#ifndef V_EXTERNAL_LOCAL_PROTO_H
#define V_EXTERNAL_LOCAL_PROTO_H

struct _options {
    struct Option *dsn, *output, *layer;
};

struct _flags {
    struct Flag *format, *layer, *tlist, *topo, *list;
};

/* args.c */
void parse_args(int, char **,
		struct _options *, struct _flags*);

/* list.c */
void list_formats();
int list_layers(FILE *, const char *, const char *, int, int);
void get_table_name(const char *, char **, char **);

#endif
