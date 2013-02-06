/*!
 * \file gis/parser.c
 *
 * \brief GIS Library - Argument parsing functions.
 *
 * Parses the command line provided through argc and argv.  Example:
 * Assume the previous calls:
 *
 * \code
 *  opt1 = G_define_option() ;
 *  opt1->key        = "map",
 *  opt1->type       = TYPE_STRING,
 *  opt1->required   = YES,
 *  opt1->checker    = sub,
 *  opt1->description= "Name of an existing raster map" ;
 *
 *  opt2 = G_define_option() ;
 *  opt2->key        = "color",
 *  opt2->type       = TYPE_STRING,
 *  opt2->required   = NO,
 *  opt2->answer     = "white",
 *  opt2->options    = "red,orange,blue,white,black",
 *  opt2->description= "Color used to display the map" ;
 *
 *  opt3 = G_define_option() ;
 *  opt3->key        = "number",
 *  opt3->type       = TYPE_DOUBLE,
 *  opt3->required   = NO,
 *  opt3->answer     = "12345.67",
 *  opt3->options    = "0-99999",
 *  opt3->description= "Number to test parser" ;
 * \endcode
 *
 * G_parser() will respond to the following command lines as described:
 *
 * \verbatim
 * command      (No command line arguments)
 * \endverbatim
 *    Parser enters interactive mode.
 *
 * \verbatim
 * command map=map.name
 * \endverbatim
 *    Parser will accept this line.  Map will be set to "map.name", the
 *    'a' and 'b' flags will remain off and the num option will be set
 *    to the default of 5.
 *
 * \verbatim
 * command -ab map=map.name num=9
 * command -a -b map=map.name num=9
 * command -ab map.name num=9
 * command map.name num=9 -ab
 * command num=9 -a map=map.name -b
 * \endverbatim
 *    These are all treated as acceptable and identical. Both flags are
 *    set to on, the map option is "map.name" and the num option is "9".
 *    Note that the "map=" may be omitted from the command line if it
 *    is part of the first option (flags do not count).
 *
 * \verbatim
 * command num=12
 * \endverbatim
 *    This command line is in error in two ways.  The user will be told
 *    that the "map" option is required and also that the number 12 is
 *    out of range.  The acceptable range (or list) will be printed.
 *
 * (C) 2001-2009, 2011 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 * \author Soeren Gebbert added Dec. 2009 WPS process_description document
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/spawn.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"

enum opt_error {
    BAD_SYNTAX    = 1,
    OUT_OF_RANGE  = 2,
    MISSING_VALUE = 3,
    AMBIGUOUS     = 4,
    REPLACED      = 5
};

#define KEYLENGTH 64

/* initialize the global struct */
struct state state;
struct state *st = &state;

/* local prototypes */
static void set_flag(int);
static int contains(const char *, int);
static int is_option(const char *);
static void set_option(const char *);
static void check_opts(void);
static void check_an_opt(const char *, int, const char *, const char **, char **);
static int check_int(const char *, const char **);
static int check_double(const char *, const char **);
static int check_string(const char *, const char **, int *);
static void check_required(void);
static void split_opts(void);
static void check_multiple_opts(void);
static int check_overwrite(void);
static void define_keywords(void);
static void split_gisprompt(const char *gisprompt, char *age, char *element, char *desc);
static void module_gui_wx(void);
static void append_error(const char *);

/*!
 * \brief Disables the ability of the parser to operate interactively.
 *
 * When a user calls a command with no arguments on the command line,
 * the parser will enter its own standardized interactive session in
 * which all flags and options are presented to the user for input. A
 * call to G_disable_interactive() disables the parser's interactive
 * prompting.
 *
 */

void G_disable_interactive(void)
{
    st->no_interactive = 1;
}

/*!
 * \brief Initializes a Flag struct.
 *
 * Allocates memory for the Flag structure and returns a pointer to
 * this memory.
 *
 * Flags are always represented by single letters.  A user "turns them
 * on" at the command line using a minus sign followed by the
 * character representing the flag.
 *
 * \return Pointer to a Flag struct
 */
struct Flag *G_define_flag(void)
{
    struct Flag *flag;
    struct Item *item;

    /* Allocate memory if not the first flag */

    if (st->n_flags) {
	flag = G_malloc(sizeof(struct Flag));
	st->current_flag->next_flag = flag;
    }
    else
	flag = &st->first_flag;

    /* Zero structure */

    G_zero(flag, sizeof(struct Flag));

    st->current_flag = flag;
    st->n_flags++;

    if (st->n_items) {
	item = G_malloc(sizeof(struct Item));
	st->current_item->next_item = item;
    }
    else
	item = &st->first_item;

    G_zero(item, sizeof(struct Item));

    item->flag = flag;
    item->option = NULL;

    st->current_item = item;
    st->n_items++;

    return (flag);
}

/*!
 * \brief Initializes an Option struct.
 *
 * Allocates memory for the Option structure and returns a pointer to
 * this memory.
 *
 * Options are provided by user on command line using the standard
 * format: <i>key=value</i>. Options identified as REQUIRED must be
 * specified by user on command line. The option string can either
 * specify a range of values (e.g. "10-100") or a list of acceptable
 * values (e.g. "red,orange,yellow"). Unless the option string is
 * NULL, user provided input will be evaluated agaist this string.
 *
 * \return pointer to an Option struct
 */
struct Option *G_define_option(void)
{
    struct Option *opt;
    struct Item *item;

    /* Allocate memory if not the first option */

    if (st->n_opts) {
	opt = G_malloc(sizeof(struct Option));
	st->current_option->next_opt = opt;
    }
    else
	opt = &st->first_option;

    /* Zero structure */
    G_zero(opt, sizeof(struct Option));

    opt->required = NO;
    opt->multiple = NO;
    
    st->current_option = opt;
    st->n_opts++;

    if (st->n_items) {
	item = G_malloc(sizeof(struct Item));
	st->current_item->next_item = item;
    }
    else
	item = &st->first_item;

    G_zero(item, sizeof(struct Item));

    item->option = opt;

    st->current_item = item;
    st->n_items++;

    return (opt);
}

/*!
 * \brief Initializes a new module.
 *
 * \return pointer to a GModule struct
 */
struct GModule *G_define_module(void)
{
    struct GModule *module;

    /* Allocate memory */
    module = &st->module_info;

    /* Zero structure */
    G_zero(module, sizeof(struct GModule));

    /* Allocate keywords array */
    define_keywords();

    return (module);
}

/*!
 * \brief Parse command line.
 *
 * The command line parameters <i>argv</i> and the number of
 * parameters <i>argc</i> from the main() routine are passed directly
 * to G_parser(). G_parser() accepts the command line input entered by
 * the user, and parses this input according to the input options
 * and/or flags that were defined by the programmer.
 *
 * <b>Note:</b> The only functions which can legitimately be called
 * before G_parser() are:
 *
 *  - G_gisinit()
 *  - G_no_gisinit()
 *  - G_define_module()
 *  - G_define_flag()
 *  - G_define_option()
 *  - G_define_standard_option()
 *  - G_disable_interactive()
 *
 * The usual order a module calls functions is:
 *
 *  # G_gisinit()
 *  # G_define_module()
 *  # G_define_flag()
 *  # G_define_option()
 *  # G_parser()
 *
 * \param argc number of arguments
 * \param argv argument list
 *
 * \return 0 on success
 * \return -1 on error and calls G_usage()
 */
int G_parser(int argc, char **argv)
{
    int need_first_opt;
    int opt_checked = 0;
    char *ptr, *tmp_name, *err;
    int i;
    struct Option *opt;
    char force_gui = FALSE;

    err = NULL;
    need_first_opt = 1;
    tmp_name = G_store(argv[0]);
    st->pgm_path = tmp_name;
    st->n_errors = 0;
    st->error = NULL;
    i = strlen(tmp_name);
    while (--i >= 0) {
	if (G_is_dirsep(tmp_name[i])) {
	    tmp_name += i + 1;
	    break;
	}
    }
    G_basename(tmp_name, "exe");
    st->pgm_name = tmp_name;

    /* Stash default answers */

    opt = &st->first_option;
    while (opt) {
	if (opt->required)
	    st->has_required = 1;

	/* Parse options */
	if (opt->options) {
	    int cnt = 0;
	    char **tokens, delm[2];

	    delm[0] = ',';
	    delm[1] = '\0';
	    tokens = G_tokenize(opt->options, delm);

	    i = 0;
	    while (tokens[i]) {
		cnt++;
		i++;
	    }

	    opt->opts = G_calloc(cnt + 1, sizeof(const char *));

	    i = 0;
	    while (tokens[i]) {
		opt->opts[i] = G_store(tokens[i]);
		i++;
	    }
	    G_free_tokens(tokens);

	    if (opt->descriptions) {
		delm[0] = ';';

		opt->descs = G_calloc(cnt + 1, sizeof(const char *));
		tokens = G_tokenize(opt->descriptions, delm);

		i = 0;
		while (tokens[i]) {
		    int j, found;

		    if (!tokens[i + 1])
			break;

		    j = 0;
		    found = 0;
		    while (opt->opts[j]) {
			if (strcmp(opt->opts[j], tokens[i]) == 0) {
			    found = 1;
			    break;
			}
			j++;
		    }
		    if (!found) {
			G_warning(_("BUG in descriptions, option '%s' in <%s> does not exist"),
				  tokens[i], opt->key);
		    }
		    else {
			opt->descs[j] = G_store(tokens[i + 1]);
		    }

		    i += 2;
		}
		G_free_tokens(tokens);
	    }
	}

	/* Copy answer */
	if (opt->multiple && opt->answers && opt->answers[0]) {
	    opt->answer = G_malloc(strlen(opt->answers[0]) + 1);
	    strcpy(opt->answer, opt->answers[0]);
	    for (i = 1; opt->answers[i]; i++) {
		opt->answer = G_realloc(opt->answer,
					strlen(opt->answer) +
					strlen(opt->answers[i]) + 2);
		strcat(opt->answer, ",");
		strcat(opt->answer, opt->answers[i]);
	    }
	}
	opt->def = opt->answer;
	opt = opt->next_opt;
    }

    /* If there are NO arguments, go interactive */

    if (argc < 2 && st->has_required && !st->no_interactive && isatty(0)) {
	module_gui_wx();
	return -1;
    }
    else if (argc < 2 && st->has_required && isatty(0)) {
      	G_usage();
	return -1;
    }
    else if (argc >= 2) {

	/* If first arg is "help" give a usage/syntax message */
	if (strcmp(argv[1], "help") == 0 ||
	    strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "--help") == 0) {
	    G_usage();
	    exit(EXIT_SUCCESS);
	}

	/* If first arg is "--help-text" give a usage/syntax message
	 * with machine-readable sentinels */
	if (strcmp(argv[1], "--help-text") == 0) {
	    G__usage_text();
	    exit(EXIT_SUCCESS);
	}

	/* If first arg is "--interface-description" then print out
	 * a xml description of the task */
	if (strcmp(argv[1], "--interface-description") == 0) {
	    G__usage_xml();
	    exit(EXIT_SUCCESS);
	}

	/* If first arg is "--html-description" then print out
	 * a html description of the task */
	if (strcmp(argv[1], "--html-description") == 0) {
	    G__usage_html();
	    exit(EXIT_SUCCESS);
	}
	
	/* If first arg is "--rest-description" then print out
        * a ReST description of the task */
	if (strcmp(argv[1], "--rest-description") == 0) {
	    G__usage_rest();
	    exit(EXIT_SUCCESS);
	}	

	/* If first arg is "--wps-process-description" then print out
	 * the wps process description of the task */
	if (strcmp(argv[1], "--wps-process-description") == 0) {
	    G__wps_print_process_description();
	    exit(EXIT_SUCCESS);
	}

	/* If first arg is "--script" then then generate
	 * g.parser boilerplate */
	if (strcmp(argv[1], "--script") == 0) {
	    G__script();
	    exit(EXIT_SUCCESS);
	}

	/* Loop thru all command line arguments */

	while (--argc) {
	    ptr = *(++argv);

	    if (strcmp(ptr, "help") == 0 ||
		strcmp(ptr, "-help") == 0 || strcmp(ptr, "--help") == 0) {
		G_usage();
		exit(EXIT_SUCCESS);
	    }

	    /* Overwrite option */
	    if (strcmp(ptr, "--o") == 0 || strcmp(ptr, "--overwrite") == 0) {
		st->overwrite = 1;
	    }

	    /* Verbose option */
	    else if (strcmp(ptr, "--v") == 0 || strcmp(ptr, "--verbose") == 0) {
		char buff[32];

		/* print everything: max verbosity level */
		st->module_info.verbose = G_verbose_max();
		sprintf(buff, "GRASS_VERBOSE=%d", G_verbose_max());
		putenv(G_store(buff));
		if (st->quiet == 1) {
		    G_warning(_("Use either --quiet or --verbose flag, not both. Assuming --verbose."));
		}
		st->quiet = -1;
	    }

	    /* Quiet option */
	    else if (strcmp(ptr, "--q") == 0 || strcmp(ptr, "--quiet") == 0) {
		char buff[32];

		/* print nothing, but errors and warnings */
		st->module_info.verbose = G_verbose_min();
		sprintf(buff, "GRASS_VERBOSE=%d", G_verbose_min());
		putenv(G_store(buff));
		if (st->quiet == -1) {
		    G_warning(_("Use either --quiet or --verbose flag, not both. Assuming --quiet."));
		}
		st->quiet = 1;	/* for passing to gui init */
	    }

	    /* Force gui to come up */
	    else if (strcmp(ptr, "--ui") == 0) {
		force_gui = TRUE;
	    }

	    /* If we see a flag */
	    else if (*ptr == '-') {
		while (*(++ptr))
		    set_flag(*ptr);

	    }
	    /* If we see standard option format (option=val) */
	    else if (is_option(ptr)) {
		set_option(ptr);
		need_first_opt = 0;
	    }

	    /* If we see the first option with no equal sign */
	    else if (need_first_opt && st->n_opts) {
		st->first_option.answer = G_store(ptr);
		st->first_option.count++;
		need_first_opt = 0;
	    }

	    /* If we see the non valid argument (no "=", just argument) */
	    else {
		G_asprintf(&err, _("Sorry <%s> is not a valid option"), ptr);
		append_error(err);
	    }

	}
    }
    
    /* Split options where multiple answers are OK */
    split_opts();

    /* Run the gui if it was specifically requested */
    if (force_gui) {
	module_gui_wx();
	return -1;
    }

    /* Check multiple options */
    check_multiple_opts();

    /* Check answers against options and check subroutines */
    if (!opt_checked)
	check_opts();

    /* Make sure all required options are set */
    if (!st->suppress_required)
	check_required();
    
    if (st->n_errors > 0) {
        if (G_verbose() > -1) {
            if (G_verbose() > G_verbose_min())
                G_usage();
            fprintf(stderr, "\n");
            for (i = 0; i < st->n_errors; i++) {
                fprintf(stderr, "%s: %s\n", _("ERROR"), st->error[i]);
            }
        }
	return -1;
    }

    if (check_overwrite())
	return -1;

    return 0;
}

/*!
 * \brief Creates command to run non-interactive.
 *
 * Creates a command-line that runs the current command completely
 * non-interactive.
 *
 * \return pointer to a char string
 */
char *G_recreate_command(void)
{
    char *buff;
    char flg[4];
    char *cur;
    const char *tmp;
    struct Flag *flag;
    struct Option *opt;
    int n, len, slen;
    int nalloced = 0;

    G_debug(3, "G_recreate_command()");

    /* Flag is not valid if there are no flags to set */

    buff = G_calloc(1024, sizeof(char));
    nalloced += 1024;
    tmp = G_program_name();
    len = strlen(tmp);
    if (len >= nalloced) {
	nalloced += (1024 > len) ? 1024 : len + 1;
	buff = G_realloc(buff, nalloced);
    }
    cur = buff;
    strcpy(cur, tmp);
    cur += len;

    if (st->n_flags) {
	flag = &st->first_flag;
	while (flag) {
	    if (flag->answer == 1) {
		flg[0] = ' ';
		flg[1] = '-';
		flg[2] = flag->key;
		flg[3] = '\0';
		slen = strlen(flg);
		if (len + slen >= nalloced) {
		    nalloced +=
			(nalloced + 1024 > len + slen) ? 1024 : slen + 1;
		    buff = G_realloc(buff, nalloced);
		    cur = buff + len;
		}
		strcpy(cur, flg);
		cur += slen;
		len += slen;
	    }
	    flag = flag->next_flag;
	}
    }

    opt = &st->first_option;
    while (opt) {
	if (opt->answer && opt->answers && opt->answers[0]) {
	    slen = strlen(opt->key) + strlen(opt->answers[0]) + 4;	/* +4 for: ' ' = " " */
	    if (len + slen >= nalloced) {
		nalloced += (nalloced + 1024 > len + slen) ? 1024 : slen + 1;
		buff = G_realloc(buff, nalloced);
		cur = buff + len;
	    }
	    strcpy(cur, " ");
	    cur++;
	    strcpy(cur, opt->key);
	    cur = strchr(cur, '\0');
	    strcpy(cur, "=");
	    cur++;
	    if (opt->type == TYPE_STRING) {
		strcpy(cur, "\"");
		cur++;
	    }
	    strcpy(cur, opt->answers[0]);
	    cur = strchr(cur, '\0');
	    len = cur - buff;
	    for (n = 1; opt->answers[n]; n++) {
		if (!opt->answers[n])
		    break;
		slen = strlen(opt->answers[n]) + 2;	/* +2 for , " */
		if (len + slen >= nalloced) {
		    nalloced +=
			(nalloced + 1024 > len + slen) ? 1024 : slen + 1;
		    buff = G_realloc(buff, nalloced);
		    cur = buff + len;
		}
		strcpy(cur, ",");
		cur++;
		strcpy(cur, opt->answers[n]);
		cur = strchr(cur, '\0');
		len = cur - buff;
	    }
	    if (opt->type == TYPE_STRING) {
		strcpy(cur, "\"");
		cur++;
		len = cur - buff;
	    }
	}
	opt = opt->next_opt;
    }

    return buff;
}

/*!
  \brief Add keyword to the list

  \param keyword keyword string
*/
void G_add_keyword(const char *keyword)
{
    if (st->n_keys >= st->n_keys_alloc) {
	st->n_keys_alloc += 10;
	st->module_info.keywords = G_realloc(st->module_info.keywords,
					     st->n_keys_alloc * sizeof(char *));
    }

    st->module_info.keywords[st->n_keys++] = G_store(keyword);
}

/*!
  \brief Set keywords from the string

  \param keywords keywords separated by commas
*/
void G_set_keywords(const char *keywords)
{
    char **tokens = G_tokenize(keywords, ",");
    st->module_info.keywords = (const char **)tokens;
    st->n_keys = st->n_keys_alloc = G_number_of_tokens(tokens);
}


int G__uses_new_gisprompt(void)
{
    struct Option *opt;
    char age[KEYLENGTH];
    char element[KEYLENGTH];
    char desc[KEYLENGTH];

    if (st->module_info.overwrite)
	return 1;

    /* figure out if any of the options use a "new" gisprompt */
    /* This is to see if we should spit out the --o flag      */
    if (st->n_opts) {
	opt = &st->first_option;
	while (opt) {
	    if (opt->gisprompt) {
		split_gisprompt(opt->gisprompt, age, element, desc);
		if (strcmp(age, "new") == 0)
		    return 1;
	    }
	    opt = opt->next_opt;
	}
    }

    return 0;
}

void G__print_keywords(FILE *fd, void (*format)(FILE *, const char *))
{
    int i;

    for(i = 0; i < st->n_keys; i++) {
	if (!format) {
	    fprintf(fd, "%s", st->module_info.keywords[i]);
	}
	else {
	    format(fd, st->module_info.keywords[i]);
	}
	if (i < st->n_keys - 1)
	    fprintf(fd, ", ");
    }

    fflush(fd);
}

/*!
  \brief Get overwrite value

  \return 1 overwrite enabled
  \return 0 overwrite disabled
*/
int G_get_overwrite()
{
    return st->overwrite;
}

void define_keywords(void)
{
    st->n_keys = 0;
    st->n_keys_alloc = 0;
}

/**************************************************************************
 *
 * The remaining routines are all local (static) routines used to support
 * the parsing process.
 *
 **************************************************************************/

/*!
  \brief Invoke GUI dialog
*/
static void module_gui_wx(void)
{
    char script[GPATH_MAX];

    if (!st->pgm_path)
	st->pgm_path = G_program_name();
    if (!st->pgm_path)
	G_fatal_error(_("Unable to determine program name"));

    sprintf(script, "%s/etc/gui/wxpython/gui_core/forms.py",
	    getenv("GISBASE"));
    G_spawn(getenv("GRASS_PYTHON"), getenv("GRASS_PYTHON"), script, G_recreate_command(), NULL);
}

static void set_flag(int f)
{
    struct Flag *flag;
    char *err;
    
    err = NULL;

    /* Flag is not valid if there are no flags to set */
    if (!st->n_flags) {
	G_asprintf(&err, _("Sorry, <%c> is not a valid flag"), f);
	append_error(err);
	return;
    }

    /* Find flag with corrrect keyword */
    flag = &st->first_flag;
    while (flag) {
	if (flag->key == f) {
	    flag->answer = 1;
	    if (flag->suppress_required)
		st->suppress_required = 1;
	    return;
	}
	flag = flag->next_flag;
    }

    G_asprintf(&err, _("Sorry, <%c> is not a valid flag"), f);
    append_error(err);
}

/* contents() is used to find things strings with characters like commas and
 * dashes.
 */
static int contains(const char *s, int c)
{
    while (*s) {
	if (*s == c)
	    return TRUE;
	s++;
    }
    return FALSE;
}

static int is_option(const char *string)
{
    int n = strspn(string, "abcdefghijklmnopqrstuvwxyz0123456789_");

    return n > 0 && string[n] == '=' && string[0] != '_' && string[n-1] != '_';
}

static int match_option_1(const char *string, const char *option)
{
    const char *next;

    if (*string == '\0')
	return 1;

    if (*option == '\0')
	return 0;

    if (*string == *option && match_option_1(string + 1, option + 1))
	return 1;

    if (*option == '_' && match_option_1(string, option + 1))
	return 1;

    next = strchr(option, '_');
    if (!next)
	return 0;

    if (*string == '_')
	return match_option_1(string + 1, next + 1);

    return match_option_1(string, next + 1);
}

static int match_option(const char *string, const char *option)
{
    return (*string == *option)
	&& match_option_1(string + 1, option + 1);
}

static void set_option(const char *string)
{
    struct Option *at_opt = NULL;
    struct Option *opt = NULL;
    int got_one;
    size_t key_len;
    char the_key[KEYLENGTH];
    char *ptr, *err;

    err = NULL;

    for (ptr = the_key; *string != '='; ptr++, string++)
	*ptr = *string;
    *ptr = '\0';
    string++;

    /* Find option with best keyword match */
    got_one = 0;
    key_len = strlen(the_key);
    for (at_opt = &st->first_option; at_opt; at_opt = at_opt->next_opt) {
	if (!at_opt->key)
	    continue;
#if 1
	if (!match_option(the_key, at_opt->key))
	    continue;
#else
	if (strncmp(the_key, at_opt->key, key_len))
	    continue;
#endif

	got_one++;
	opt = at_opt;

	/* changed 1/15/91 -dpg   old code is in parser.old */
	/* overide ambiguous check, if we get an exact match */
	if (strlen(at_opt->key) == key_len) {
	    opt = at_opt;
	    got_one = 1;
	    break;
	}
    }

    if (got_one > 1) {
	G_asprintf(&err, _("Sorry, <%s=> is ambiguous"), the_key);
	append_error(err);
	return;
    }

    /* If there is no match, complain */
    if (got_one == 0) {
	G_asprintf(&err, _("Sorry, <%s> is not a valid parameter"), the_key);
	append_error(err);
	return;
    }

    /* Allocate memory where answer is stored */
    if (opt->count++) {
	if (!opt->multiple) {
	    G_asprintf(&err, _("Option <%s> does not accept multiple answers"), opt->key);
	    append_error(err);
	}
	opt->answer = G_realloc(opt->answer,
				strlen(opt->answer) + strlen(string) + 2);
	strcat(opt->answer, ",");
	strcat(opt->answer, string);
    }
    else
	opt->answer = G_store(string);
}

static void check_opts(void)
{
    struct Option *opt;
    int ans;

    if (!st->n_opts)
	return;

    opt = &st->first_option;
    while (opt) {
	/* Check answer against options if any */

	if (opt->answer) {
	    if (opt->multiple == 0)
		check_an_opt(opt->key, opt->type,
			     opt->options, opt->opts, &opt->answer);
	    else {
		for (ans = 0; opt->answers[ans] != '\0'; ans++)
		    check_an_opt(opt->key, opt->type,
				 opt->options, opt->opts, &opt->answers[ans]);
	    }
	}

	/* Check answer against user's check subroutine if any */

	if (opt->checker)
	    opt->checker(opt->answer);

	opt = opt->next_opt;
    }
}

static void check_an_opt(const char *key, int type, const char *options,
			const char **opts, char **answerp)
{
    const char *answer = *answerp;
    int error;
    char *err;
    int found;

    error = 0;
    err = NULL;

    switch (type) {
    case TYPE_INTEGER:
	error = check_int(answer, opts);
	break;
    case TYPE_DOUBLE:
	error = check_double(answer, opts);
	break;
    case TYPE_STRING:
	error = check_string(answer, opts, &found);
	break;
    }
    switch (error) {
    case 0:
	break;
    case BAD_SYNTAX:
	G_asprintf(&err,
		   _("Illegal range syntax for parameter <%s>\n"
		     "\tPresented as: %s"), key, options);
	append_error(err);
	break;
    case OUT_OF_RANGE:
	G_asprintf(&err, 
		   _("Value <%s> out of range for parameter <%s>\n"
		     "\tLegal range: %s"), answer, key, options);
	append_error(err);
	break;
    case MISSING_VALUE:
	G_asprintf(&err,
		   _("Missing value for parameter <%s>"),
		   key);
	append_error(err);
	break;
    case AMBIGUOUS:
	G_asprintf(&err,
		   _("Value <%s> ambiguous for parameter <%s>\n"
		     "\tValid options: %s"), answer, key, options);
	append_error(err);
	break;
    case REPLACED:
	*answerp = G_store(opts[found]);
	error = 0;
	break;
    }
}

static int check_int(const char *ans, const char **opts)
{
    int d, i;

    /* "-" is reserved for standard input */
    if (strcmp(ans, "-") == 0)
	return 0;

    if (sscanf(ans, "%d", &d) != 1)
	return MISSING_VALUE;

    if (!opts)
	return 0;

    for (i = 0; opts[i]; i++) {
	const char *opt = opts[i];
	int lo, hi;

	if (contains(opt, '-')) {
	    if (sscanf(opt, "%d-%d", &lo, &hi) == 2) {
		if (d >= lo && d <= hi)
		    return 0;
	    }
	    else if (sscanf(opt, "-%d", &hi) == 1) {
		if (d <= hi)
		    return 0;
	    }
	    else if (sscanf(opt, "%d-", &lo) == 1) {
		if (d >= lo)
		    return 0;
	    }
	    else
		return BAD_SYNTAX;
	}
	else {
	    if (sscanf(opt, "%d", &lo) == 1) {
		if (d == lo)
		    return 0;
	    }
	    else
		return BAD_SYNTAX;
	}
    }

    return OUT_OF_RANGE;
}

static int check_double(const char *ans, const char **opts)
{
    double d;
    int i;

    /* "-" is reserved for standard input */
    if (strcmp(ans, "-") == 0)
	return 0;
    
    if (sscanf(ans, "%lf", &d) != 1)
	return MISSING_VALUE;

    if (!opts)
	return 0;

    for (i = 0; opts[i]; i++) {
	const char *opt = opts[i];
	double lo, hi;

	if (contains(opt, '-')) {
	    if (sscanf(opt, "%lf-%lf", &lo, &hi) == 2) {
		if (d >= lo && d <= hi)
		    return 0;
	    }
	    else if (sscanf(opt, "-%lf", &hi) == 1) {
		if (d <= hi)
		    return 0;
	    }
	    else if (sscanf(opt, "%lf-", &lo) == 1) {
		if (d >= lo)
		    return 0;
	    }
	    else
		return BAD_SYNTAX;
	}
	else {
	    if (sscanf(opt, "%lf", &lo) == 1) {
		if (d == lo)
		    return 0;
	    }
	    else
		return BAD_SYNTAX;
	}
    }

    return OUT_OF_RANGE;
}

static int check_string(const char *ans, const char **opts, int *result)
{
    int len = strlen(ans);
    int found = 0;
    int i;

    if (!opts)
	return 0;

    for (i = 0; opts[i]; i++) {
	if (strcmp(ans, opts[i]) == 0)
	    return 0;
	if (strncmp(ans, opts[i], len) == 0) {
	    *result = i;
	    found++;
	}
    }

    switch (found) {
    case 0: return OUT_OF_RANGE;
    case 1: return REPLACED;
    default: return AMBIGUOUS;
    }
}

static void check_required(void)
{
    struct Option *opt;
    char *err;
    
    err = NULL;
    
    if (!st->n_opts)
	return;

    opt = &st->first_option;
    while (opt) {
	if (opt->required && !opt->answer) {
	    G_asprintf(&err, _("Required parameter <%s> not set:\n"
			       "\t(%s)"),
		       opt->key, (opt->label ? opt->label : opt->description));
	    append_error(err);
	}
	opt = opt->next_opt;
    }
}

static void split_opts(void)
{
    struct Option *opt;
    const char *ptr1;
    const char *ptr2;
    int allocated;
    int ans_num;
    int len;


    if (!st->n_opts)
	return;

    opt = &st->first_option;
    while (opt) {
	if ( /*opt->multiple && */ opt->answer) {
	    /* Allocate some memory to store array of pointers */
	    allocated = 10;
	    opt->answers = G_malloc(allocated * sizeof(char *));

	    ans_num = 0;
	    ptr1 = opt->answer;
	    opt->answers[ans_num] = NULL;

	    for (;;) {
		for (len = 0, ptr2 = ptr1; *ptr2 != '\0' && *ptr2 != ',';
		     ptr2++, len++) ;

		if (len > 0) {	/* skip ,, */
		    opt->answers[ans_num] = G_malloc(len + 1);
		    memcpy(opt->answers[ans_num], ptr1, len);
		    opt->answers[ans_num][len] = 0;

		    ans_num++;

		    if (ans_num >= allocated) {
			allocated += 10;
			opt->answers = G_realloc(opt->answers,
						 allocated * sizeof(char *));
		    }

		    opt->answers[ans_num] = NULL;
		}

		if (*ptr2 == '\0')
		    break;

		ptr1 = ptr2 + 1;

		if (*ptr1 == '\0')
		    break;
	    }
	}
	opt = opt->next_opt;
    }
}

static void check_multiple_opts(void)
{
    struct Option *opt;
    const char *ptr;
    int n_commas;
    int n;
    char *err;

    if (!st->n_opts)
	return;

    err = NULL;
    opt = &st->first_option;
    while (opt) {
	/* "-" is reserved from standard input/output */
	if (opt->answer && strcmp(opt->answer, "-") && opt->key_desc) {
	    /* count commas */
	    n_commas = 1;
	    for (ptr = opt->key_desc; *ptr != '\0'; ptr++)
		if (*ptr == ',')
		    n_commas++;
	    /* count items */
	    for (n = 0; opt->answers[n] != '\0'; n++) ;
	    /* if not correct multiple of items */
	    if (n % n_commas) {
		G_asprintf(&err,
			   _("Option <%s> must be provided in multiples of %d\n"
			     "\tYou provided %d item(s): %s"),
			   opt->key, n_commas, n, opt->answer);
		append_error(err);
			   
	    }
	}
	opt = opt->next_opt;
    }
}

/* Check for all 'new' if element already exists */
static int check_overwrite(void)
{
    struct Option *opt;
    char age[KEYLENGTH];
    char element[KEYLENGTH];
    char desc[KEYLENGTH];
    int error = 0;
    const char *overstr;
    int over;

    st->module_info.overwrite = 0;

    if (!st->n_opts)
	return (0);

    over = 0;
    /* Check the GRASS OVERWRITE variable */
    if ((overstr = G__getenv("OVERWRITE"))) {
	over = atoi(overstr);
    }

    /* Check the GRASS_OVERWRITE environment variable */
    if ((overstr = getenv("GRASS_OVERWRITE"))) {
	if (atoi(overstr))
	    over = 1;
    }

    if (st->overwrite || over) {
	st->module_info.overwrite = 1;
	/* Set the environment so that programs run in a script also obey --o */
	putenv("GRASS_OVERWRITE=1");
	/* No need to check options for existing files if overwrite is true */
	return error;
    }

    opt = &st->first_option;
    while (opt) {
	if (opt->answer && opt->gisprompt) {
	    split_gisprompt(opt->gisprompt, age, element, desc);

	    if (strcmp(age, "new") == 0) {
		int i;
		char found;
		
		for (i = 0; opt->answers[i]; i++) {
		    found = FALSE;
		    if (strcmp(element, "file") == 0) {
                        if (access(opt->answers[i], F_OK) == 0)
                            found = TRUE;
		    }
                    else if (G_find_file(element, opt->answers[i], G_mapset())) {
			found = TRUE;
		    }
                    
		    if (found) {	/* found */
			if (!st->overwrite && !over) {
                            if (G_verbose() > -1) {
                                if (G_info_format() != G_INFO_FORMAT_GUI) {
                                    fprintf(stderr,
                                            _("ERROR: option <%s>: <%s> exists.\n"),
                                            opt->key, opt->answers[i]);
                                }
                                else {
                                    fprintf(stderr,
                                            "GRASS_INFO_ERROR(%d,1): option <%s>: <%s> exists.\n",
                                            getpid(), opt->key, opt->answers[i]);
                                    fprintf(stderr, "GRASS_INFO_END(%d,1)\n",
                                            getpid());
                                }
                            }
			    error = 1;
			}
		    }
		}
	    }
	}
	opt = opt->next_opt;
    }

    return (error);
}

static void split_gisprompt(const char *gisprompt, char *age, char *element,
			    char *desc)
{
    const char *ptr1;
    char *ptr2;

    for (ptr1 = gisprompt, ptr2 = age; *ptr1 != '\0'; ptr1++, ptr2++) {
	if (*ptr1 == ',')
	    break;
	*ptr2 = *ptr1;
    }
    *ptr2 = '\0';

    for (ptr1++, ptr2 = element; *ptr1 != '\0'; ptr1++, ptr2++) {
	if (*ptr1 == ',')
	    break;
	*ptr2 = *ptr1;
    }
    *ptr2 = '\0';

    for (ptr1++, ptr2 = desc; *ptr1 != '\0'; ptr1++, ptr2++) {
	if (*ptr1 == ',')
	    break;
	*ptr2 = *ptr1;
    }
    *ptr2 = '\0';
}

static void append_error(const char *msg)
{
    st->error = G_realloc(st->error, sizeof(char *) * (st->n_errors + 1));
    st->error[st->n_errors++] = G_store(msg);
}

/*!
  \brief Get separator string from the option.

  Calls G_fatal_error() on error. Allocated string can be later freed
  by G_free().
  
  \code
  char *fs;
  struct Option *opt_fs;
  
  opt_fs = G_define_standard_option(G_OPT_F_SEP);
  
  if (G_parser(argc, argv))
      exit(EXIT_FAILURE);
      
  fs = G_option_to_separator(opt_fs);
  \endcode

  \param option pointer to separator option
  
  \return allocated string with separator
*/
char* G_option_to_separator(const struct Option *option)
{
    char* sep;
    
    if (option->answer == NULL)
        G_fatal_error(_("No separator given"));

    if (strcmp(option->answer, "space") == 0)
	sep = G_store(" ");
    else if (strcmp(option->answer, "tab") == 0 ||
             strcmp(option->answer, "\\t") == 0)
        sep = G_store("\t");
    else if (strcmp(option->answer, "newline") == 0)
        sep = G_store("\n");
    else if (strcmp(option->answer, "comma") == 0)
        sep = G_store(",");
    else {
        sep = G_store(option->answer);
    }
    
    G_debug(1, "G_option_to_separator(): key = %s -> sep = '%s'",
	    option->key, sep);
    
    return sep;
}
