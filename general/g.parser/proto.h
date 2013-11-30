#include "global.h"

/* parse.c */
int parse_boolean(struct context *, const char *);
void parse_toplevel(struct context *, const char *);
void parse_module(struct context *, const char *, const char *);
void parse_flag(struct context *, const char *, const char *);
int parse_type(struct context *, const char *);
void parse_option(struct context *, const char *, const char *);
int print_options(const struct context *, int);

/* revoke.c */
int reinvoke_script(const struct context *, const char *);

/* standard_option.c */
struct Option *define_standard_option(const char *);

/* translate.c */
char *translate(const char *);
