#ifndef __PARSER_LOCAL_PROTO_H__
#define __PARSER_LOCAL_PROTO_H__

#include <stdio.h>
#include <grass/gis.h>

#define KEYLENGTH 64

struct Item
{
    struct Option *option;
    struct Flag *flag;
    struct Item *next_item;
};

struct state {
    int no_interactive;
    int n_opts;
    int n_flags;
    int n_keys;
    int n_keys_alloc;
    int overwrite;
    int quiet;
    int has_required;
    int suppress_required;
    int suppress_overwrite;

    struct GModule module_info;	/* general information on the corresponding module */

    const char *pgm_name;
    const char *pgm_path;

    struct Flag first_flag;	/* First flag in a linked list      */
    struct Flag *current_flag;	/* Pointer for traversing list      */

    struct Option first_option;
    struct Option *current_option;

    struct Item first_item;
    struct Item *current_item;
    int n_items;

    char **error;
    int n_errors;

    struct Key_Value *renamed_options;
};

extern struct state *st;

/* functions which are used by several parser functions in different files */

void G__usage_xml(void);
void G__usage_html(void);
void G__usage_rest(void);
void G__usage_text(void);
void G__script(void);
char *G__json(void);
void G__wps_print_process_description(void);
int  G__uses_new_gisprompt(void);
void G__print_keywords(FILE *, void (*)(FILE *, const char *));
void G__split_gisprompt(const char *, char *, char *, char *);

void G__check_option_rules(void);
void G__describe_option_rules(void);
int G__has_required_rule(void);
void G__describe_option_rules_xml(FILE *);

#endif

