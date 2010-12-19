#ifndef V_EXTERNAL_OUT_LOCAL_PROTO_H
#define V_EXTERNAL_OUT_LOCAL_PROTO_H

struct _options {
    struct Option *dsn, *format, *opts;
};

struct _flags {
    struct Flag *f, *p, *r;
};

/* args.c */
void parse_args(int, char **,
		struct _options *, struct _flags*);

/* format.c */
void check_format(const char *);

/* link.c */
void make_link(const char *,
	       const char *, char **);

/* list.c */
char *format_list(void);
void list_formats(void);

/* status.c */
void print_status(void);

#endif
