#ifndef V_EXTERNAL_OUT_LOCAL_PROTO_H
#define V_EXTERNAL_OUT_LOCAL_PROTO_H

struct _options {
    struct Option *dsn, *format, *opts, *input, *output;
};

struct _flags {
    struct Flag *f, *p, *r, *g;
};

/* args.c */
void parse_args(int, char **,
		struct _options *, struct _flags*);

/* format.c */
int check_format(char *);
int is_ogr(const char *);

/* link.c */
void make_link(const char *,
	       const char *, char *, char **);

/* list.c */
char *format_options(void);
void list_formats(void);

/* status.c */
void print_status(int);
int save_status_file(const struct Option *);
int read_status_file(const struct Option *);

#endif
