#ifndef __PARSER_LOCAL_PROTO_H__
#define __PARSER_LOCAL_PROTO_H__

#include <grass/config.h>

#if defined(HAVE_LANGINFO_H)
#include <langinfo.h>
#endif
#if defined(__MINGW32__) && defined(USE_NLS)
#include <localcharset.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/spawn.h>

struct Item
{
    struct Option *option;
    struct Flag *flag;
    struct Item *next_item;
};

typedef struct state {
    int no_interactive;
    int n_opts;
    int n_flags;
    int n_keys;
    int n_keys_alloc;
    int overwrite;
    int quiet;
    int has_required;

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
}state_type;


extern state_type *st;

#define BAD_SYNTAX    1
#define OUT_OF_RANGE  2
#define MISSING_VALUE 3
#define AMBIGUOUS     4
#define REPLACED      5
#define KEYLENGTH 64

/* functions which are used by several parser functions in different files */

void usage_xml(void);
void usage_html(void);
void script(void);
void wps_print_process_description(void);
void print_escaped_for_xml(FILE * fp, const char *str);
int  uses_new_gisprompt(void);
void print_keywords(FILE *fd, void (*format)(FILE *, const char *));

#endif