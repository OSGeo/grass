
/**
 * \file parser.c
 *
 * \brief GIS Library - Argument parsing functions.
 *
 * Parses the command line provided through argc and argv.  Example:
 * Assume the previous calls:
 *
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
 *
 * G_parser() will respond to the following command lines as described:
 *
 * command      (No command line arguments)
 *    Parser enters interactive mode.
 *
 * command map=map.name
 *    Parser will accept this line.  Map will be set to "map.name", the
 *    'a' and 'b' flags will remain off and the num option will be set
 *    to the default of 5.
 *
 * command -ab map=map.name num=9
 * command -a -b map=map.name num=9
 * command -ab map.name num=9
 * command map.name num=9 -ab
 * command num=9 -a map=map.name -b
 *    These are all treated as acceptable and identical. Both flags are
 *    set to on, the map option is "map.name" and the num option is "9".
 *    Note that the "map=" may be omitted from the command line if it
 *    is part of the first option (flags do not count).
 *
 * command num=12
 *    This command line is in error in two ways.  The user will be told 
 *    that the "map" option is required and also that the number 12 is
 *    out of range.  The acceptable range (or list) will be printed.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 *
 * \date 2003-2008
 *
 */

#include <grass/config.h>

#if defined(HAVE_LANGINFO_H)
#include <langinfo.h>
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


#define BAD_SYNTAX  1
#define OUT_OF_RANGE    2
#define MISSING_VALUE   3
#define KEYLENGTH 64

static int interactive_ok = 1;
static int n_opts = 0;
static int n_flags = 0;
static int overwrite = 0;
static int quiet = 0;

static struct Flag first_flag;	/* First flag in a linked list      */
static struct Flag *current_flag;	/* Pointer for traversing list      */

static struct Option first_option;
static struct Option *current_option;

static struct GModule module_info;	/* general information on the corresponding module */

static const char *pgm_name = NULL;

struct Item
{
    struct Option *option;
    struct Flag *flag;
    struct Item *next_item;
};

static struct Item first_item;
static struct Item *current_item;
static int n_items = 0;
static int show_options(int, const char *);
static int show(const char *, int);
static int set_flag(int);
static int contains(const char *, int);
static int is_option(const char *);
static int set_option(char *);
static int check_opts();
static int check_an_opt(const char *, int, const char *, const char *);
static int check_int(const char *, const char *);
static int check_double(const char *, const char *);
static int check_string(const char *, const char *);
static int check_required(void);
static int split_opts(void);
static int check_multiple_opts(void);
static int check_overwrite(void);
static int split_gisprompt(const char *, char *, char *, char *);

static void G_gui(void);
static void G_tcltk(void);
static void G_usage_xml(void);
static void G_usage_html(void);
static void G_script(void);


/**
 * \brief Disables the ability of the parser to operate interactively.
 *
 * When a user calls a command with no arguments on the command line, 
 * the parser will enter its own standardized interactive session in 
 * which all flags and options  are presented to the user for input. A 
 * call to <i>G_disable_interactive()</i> disables the parser's 
 * interactive prompting.
 *
 * \return always returns 0
 */

int G_disable_interactive(void)
{
    interactive_ok = 0;

    return 0;
}


/**
 * \brief Initializes a Flag struct.
 *
 * Allocates memory for the Flag structure and returns a pointer to this 
 * memory (of <i>type struct Flag *</i>).<br>
 *
 * Flags are always represented by single letters.  A user "turns them on"
 * at the command line using a minus sign followed by the character
 * representing the flag.
 *
 * \return Flag * Pointer to a Flag struct
 */

struct Flag *G_define_flag(void)
{
    struct Flag *flag;
    struct Item *item;

    /* Allocate memory if not the first flag */

    if (n_flags) {
	flag = (struct Flag *)G_malloc(sizeof(struct Flag));
	current_flag->next_flag = flag;
    }
    else
	flag = &first_flag;

    /* Zero structure */

    G_zero((char *)flag, sizeof(struct Flag));

    current_flag = flag;
    n_flags++;

    if (n_items) {
	item = (struct Item *)G_malloc(sizeof(struct Item));
	current_item->next_item = item;
    }
    else
	item = &first_item;

    G_zero((char *)item, sizeof(struct Item));

    item->flag = flag;
    item->option = NULL;

    current_item = item;
    n_items++;

    return (flag);
}


/**
 * \brief Initializes an Option struct.
 *
 * Allocates memory for the Option structure and returns a pointer to
 * this memory (of <i>type struct Option *</i>).<br>
 *
 * Options are provided by user on command line using the standard
 * format: <i>key=value</i>. Options identified as REQUIRED must be 
 * specified by user on command line. The option string can either 
 * specify a range of values (e.g. "10-100") or a list of acceptable 
 * values (e.g. "red,orange,yellow").  Unless the option string is NULL, 
 * user provided input will be evaluated agaist this string.
 *
 * \return Option * Pointer to an Option struct
 */

struct Option *G_define_option(void)
{
    struct Option *opt;
    struct Item *item;

    /* Allocate memory if not the first option */

    if (n_opts) {
	opt = (struct Option *)G_malloc(sizeof(struct Option));
	current_option->next_opt = opt;
    }
    else
	opt = &first_option;

    /* Zero structure */
    G_zero((char *)opt, sizeof(struct Option));

    opt->required = NO;
    opt->multiple = NO;
    opt->answer = NULL;
    opt->answers = NULL;
    opt->def = NULL;
    opt->checker = NULL;
    opt->options = NULL;
    opt->key_desc = NULL;
    opt->gisprompt = NULL;
    opt->label = NULL;
    opt->opts = NULL;
    opt->description = NULL;
    opt->descriptions = NULL;
    opt->guisection = NULL;

    current_option = opt;
    n_opts++;

    if (n_items) {
	item = (struct Item *)G_malloc(sizeof(struct Item));
	current_item->next_item = item;
    }
    else
	item = &first_item;

    G_zero((char *)item, sizeof(struct Item));

    item->option = opt;
    item->flag = NULL;

    current_item = item;
    n_items++;

    return (opt);
}


/**
 * \brief Create standardised Option structure.
 *
 * This function will create a standardised Option structure
 * defined by parameter opt. A list of valid parameters can be found in gis.h.
 * It allocates memory for the Option structure and returns a pointer to
 * this memory (of <i>type struct Option *</i>).<br>
 *
 * If an invalid parameter was specified a empty Option structure will 
 * be returned (not NULL).
 *
 *  - general: G_OPT_WHERE, G_OPT_COLUMN, G_OPT_COLUMNS, G_OPT_TABLE, G_OPT_DRIVER, G_OPT_DATABASE
 *
 *  - imagery: G_OPT_I_GROUP, G_OPT_I_SUBGROUP
 *
 *  - raster: G_OPT_R_INPUT, G_OPT_R_INPUTS, G_OPT_R_OUTPUT, G_OPT_R_MAP, G_OPT_R_MAPS, G_OPT_R_BASE, G_OPT_R_COVER, G_OPT_R_ELEV, G_OPT_R_ELEVS
 *
 *  - raster3d: G_OPT_R3_INPUT, G_OPT_R3_INPUTS, G_OPT_R3_OUTPUT, G_OPT_R3_MAP, G_OPT_R3_MAPS
 *
 *  - vector: G_OPT_V_INPUT, G_OPT_V_INPUTS, G_OPT_V_OUTPUT, G_OPT_V_MAP, G_OPT_V_MAPS, G_OPT_V_TYPE, G_OPT_V_FIELD, G_OPT_V_CAT, G_OPT_V_CATS
 *
 * \param[in] opt Type of Option struct to create
 *
 * \return Option * Pointer to an Option struct
 */

struct Option *G_define_standard_option(int opt)
{
    struct Option *Opt;

    Opt = G_define_option();

    switch (opt) {
	/* Database options (change to G_OPT_DB_*?) */
    case G_OPT_WHERE:
	Opt->key = "where";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "sql_query";
	Opt->required = NO;
	Opt->label =
	    _("WHERE conditions of SQL statement without 'where' keyword");
	Opt->description = _("Example: income < 1000 and inhab >= 10000");
	break;
    case G_OPT_TABLE:
	Opt->key = "table";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->description = _("Table name");
	break;
    case G_OPT_DRIVER:
	Opt->key = "driver";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->description = _("Driver name");
	break;
    case G_OPT_DATABASE:
	Opt->key = "database";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->description = _("Database name");
	break;
    case G_OPT_COLUMN:
	Opt->key = "column";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->description = _("Name of attribute column");
	break;
    case G_OPT_COLUMNS:
	Opt->key = "columns";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = YES;
	Opt->description = _("Name of attribute column(s)");
	break;

	/* imagery group */
    case G_OPT_I_GROUP:
	Opt->key = "group";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,group,group";
	Opt->description = _("Name of input imagery group");
	break;
    case G_OPT_I_SUBGROUP:
	Opt->key = "subgroup";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,subgroup,subgroup";
	Opt->description = _("Name of input imagery subgroup");
	break;

	/* raster maps */
    case G_OPT_R_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of input raster map");
	break;
    case G_OPT_R_INPUTS:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of input raster map(s)");
	break;
    case G_OPT_R_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new,cell,raster";
	Opt->description = _("Name for output raster map");
	break;
    case G_OPT_R_MAP:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of input raster map");
	break;
    case G_OPT_R_MAPS:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of input raster map(s)");
	break;
    case G_OPT_R_BASE:
	Opt->key = "base";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of base raster map");
	break;
    case G_OPT_R_COVER:
	Opt->key = "cover";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of cover raster map");
	break;
    case G_OPT_R_ELEV:
	Opt->key = "elevation";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of elevation raster map");
	break;
    case G_OPT_R_ELEVS:
	Opt->key = "elevation";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of elevation raster map(s)");
	break;

	/*g3d maps */
    case G_OPT_R3_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,grid3,3d-raster";
	Opt->description = _("Name of input raster3d map");
	break;
    case G_OPT_R3_INPUTS:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,grid3,3d-raster";
	Opt->description = _("Name of input raster3d map(s)");
	break;
    case G_OPT_R3_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new,grid3,3d-raster";
	Opt->description = _("Name for output raster3d map");
	break;
    case G_OPT_R3_MAP:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,grid3,3d-raster";
	Opt->description = _("Name of input raster3d map");
	break;
    case G_OPT_R3_MAPS:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,grid3,3d-raster";
	Opt->description = _("Name of input raster3d map(s)");
	break;

	/*vector maps */
    case G_OPT_V_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,vector,vector";
	Opt->description = _("Name of input vector map");
	break;
    case G_OPT_V_INPUTS:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,vector,vector";
	Opt->description = _("Name of input vector map(s)");
	break;
    case G_OPT_V_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new,vector,vector";
	Opt->description = _("Name for output vector map");
	break;
    case G_OPT_V_MAP:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,vector,vector";
	Opt->description = _("Name of input vector map");
	break;
    case G_OPT_V_MAPS:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,vector,vector";
	Opt->description = _("Name of input vector map(s)");
	break;
    case G_OPT_V_TYPE:
	Opt->key = "type";
	Opt->type = TYPE_STRING;
	Opt->required = NO;
	Opt->multiple = YES;
	Opt->answer = "point,line,boundary,centroid,area";
	Opt->options = "point,line,boundary,centroid,area";
	Opt->label = _("Type");
	Opt->description = _("Feature type(s)");
	break;
    case G_OPT_V_FIELD:
	Opt->key = "layer";
	Opt->type = TYPE_INTEGER;
	Opt->required = NO;
	Opt->answer = "1";
	Opt->label = _("Layer number");
	Opt->description =
	    _("A single vector map can be connected to multiple database "
	      "tables. This number determines which table to use.");
	break;
    case G_OPT_V_CAT:
	Opt->key = "cat";
	Opt->type = TYPE_INTEGER;
	Opt->required = NO;
	Opt->description = _("Category value");
	break;
    case G_OPT_V_CATS:
	Opt->key = "cats";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "range";
	Opt->required = NO;
	Opt->label = _("Category values");
	Opt->description = _("Example: 1,3,7-9,13");
	break;

	/* files */
    case G_OPT_F_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old_file,file,input";
	Opt->description = _("Name of input file");
	break;
    case G_OPT_F_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new_file,file,output";
	Opt->description = _("Name for output file");
	break;
    case G_OPT_F_SEP:
	Opt->key = "fs";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "character";
	Opt->required = NO;
	Opt->answer = "|";
	Opt->description = _("Field separator");
	break;

	/* colors */
    case G_OPT_C_FG:
	Opt->key = "color";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->answer = DEFAULT_FG_COLOR;
	Opt->gisprompt = GISPROMPT_COLOR;
	Opt->label = _("Color");
	Opt->description = _("Either a standard color name or R:G:B triplet");
	break;
    case G_OPT_C_BG:
	Opt->key = "bgcolor";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->answer = DEFAULT_BG_COLOR;
	Opt->gisprompt = GISPROMPT_COLOR;
	Opt->label = _("Background color");
	Opt->description =
	    _("Either a standard GRASS color, R:G:B triplet, or \"none\"");
	break;
    }

    return (Opt);
}


/**
 * \brief Initializes a new module.
 *
 * \return GModule * Pointer to a GModule struct
 */

struct GModule *G_define_module(void)
{
    struct GModule *module;

    /* Allocate memory */

    module = &module_info;

    /* Zero structure */

    G_zero((char *)module, sizeof(struct GModule));

    return (module);
}

/* The main parsing routine */

/**
 * \brief Parse command line.
 *
 * The command line parameters <b>argv</b> and the number of parameters 
 * <b>argc</b> from the main() routine are passed directly to 
 * <i>G_parser()</i>. <i>G_parser()</i> accepts the command line input 
 * entered by the user, and parses this input according to the input 
 * options and/or flags that were defined by the programmer.<br>
 *
 * <b>Note:</b> The only functions which can legitimately be called 
 * before G_parser() are:<br>
 * <ul>
 *  <li>G_gisinit()</li>
 *  <li>G_no_gisinit()</li>
 *  <li>G_define_module()</li>
 *  <li>G_define_flag()</li>
 *  <li>G_define_option()</li>
 *  <li>G_define_standard_option()</li>
 *  <li>G_disable_interactive()</li>
 * </ul>
 *
 * The usual order a module calls functions is:<br>
 * <ul>
 *  <li>G_gisinit()</li>
 *  <li>G_define_module()</li>
 *  <li>G_define_{flag,option}()</li>
 *  <li>G_parser()</li>
 * </ul>
 *
 * \param[in] argc number of arguments
 * \param[in] argv argument list
 * \return 0 on success
 * \return -1 on error and calls <b>G_usage()</b>
 */

int G_parser(int argc, char **argv)
{
    int need_first_opt;
    int opt_checked = 0;
    int error;
    char *ptr, *tmp_name;
    int i;
    struct Option *opt;
    char force_gui = FALSE;

    error = 0;
    need_first_opt = 1;
    i = strlen(tmp_name = G_store(argv[0]));
    while (--i >= 0) {
	if (G_is_dirsep(tmp_name[i])) {
	    tmp_name += i + 1;
	    break;
	}
    }
    G_basename(tmp_name, "exe");
    pgm_name = tmp_name;

    /* Stash default answers */

    opt = &first_option;
    while (opt != NULL) {
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

	    opt->opts =
		(const char **)G_calloc(cnt + 1, sizeof(const char *));

	    i = 0;
	    while (tokens[i]) {
		opt->opts[i] = G_store(tokens[i]);
		i++;
	    }
	    G_free_tokens(tokens);

	    if (opt->descriptions) {
		delm[0] = ';';

		opt->descs =
		    (const char **)G_calloc(cnt + 1, sizeof(const char *));
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
			G_warning(_
				  ("BUG in descriptions, option %s in %s does not exist"),
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
	    opt->answer = (char *)G_malloc(strlen(opt->answers[0]) + 1);
	    strcpy(opt->answer, opt->answers[0]);
	    for (i = 1; opt->answers[i]; i++) {
		opt->answer = (char *)G_realloc(opt->answer,
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

    if (argc < 2 && interactive_ok && isatty(0)) {
	G_gui();
	return -1;
    }
    else if (argc < 2 && isatty(0)) {
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

	/* If first arg is "--interface-description" then print out
	 * a xml description of the task */
	if (strcmp(argv[1], "--interface-description") == 0) {
	    G_usage_xml();
	    exit(EXIT_SUCCESS);
	}

	/* If first arg is "--html-description" then print out
	 * a html description of the task */
	if (strcmp(argv[1], "--html-description") == 0) {
	    G_usage_html();
	    exit(EXIT_SUCCESS);
	}

	/* If first arg is "--tcltk" then generate
	 * code for tcltkgrass */
	if (strcmp(argv[1], "--tcltk") == 0) {
	    G_tcltk();
	    exit(EXIT_SUCCESS);
	}

	/* If first arg is "--script" then then generate
	 * g.parser boilerplate */
	if (strcmp(argv[1], "--script") == 0) {
	    G_script();
	    exit(EXIT_SUCCESS);
	}

	/* Loop thru all command line arguments */

	while (--argc) {
	    ptr = *(++argv);

	    /* Overwrite option */
	    if (strcmp(ptr, "--o") == 0 || strcmp(ptr, "--overwrite") == 0) {
		overwrite = 1;
	    }

	    /* Verbose option */
	    else if (strcmp(ptr, "--v") == 0 || strcmp(ptr, "--verbose") == 0) {
		char buff[32];

		/* print everything: max verbosity level */
		module_info.verbose = G_verbose_max();
		sprintf(buff, "GRASS_VERBOSE=%d", G_verbose_max());
		putenv(G_store(buff));
		if (quiet == 1) {
		    G_warning(_
			      ("Use either --quiet or --verbose flag, not both. Assuming --verbose."));
		}
		quiet = -1;
	    }

	    /* Quiet option */
	    else if (strcmp(ptr, "--q") == 0 || strcmp(ptr, "--quiet") == 0) {
		char buff[32];

		/* print nothing, but errors and warnings */
		module_info.verbose = G_verbose_min();
		sprintf(buff, "GRASS_VERBOSE=%d", G_verbose_min());
		putenv(G_store(buff));
		if (quiet == -1) {
		    G_warning(_
			      ("Use either --quiet or --verbose flag, not both. Assuming --quiet."));
		}
		quiet = 1;	/* for passing to gui init */
	    }

	    /* Force gui to come up */
	    else if (strcmp(ptr, "--ui") == 0) {
		force_gui = TRUE;
	    }

	    /* If we see a flag */
	    else if (*ptr == '-') {
		while (*(++ptr))
		    error += set_flag(*ptr);

	    }
	    /* If we see standard option format (option=val) */
	    else if (is_option(ptr)) {
		error += set_option(ptr);
		need_first_opt = 0;
	    }

	    /* If we see the first option with no equal sign */
	    else if (need_first_opt && n_opts) {
		first_option.answer = G_store(ptr);
		need_first_opt = 0;
	    }

	    /* If we see the non valid argument (no "=", just argument) */
	    else if (contains(ptr, '=') == 0) {
		fprintf(stderr, _("Sorry <%s> is not a valid option\n"), ptr);
		error = 1;
	    }

	}
    }

    /* Run the gui if it was specifically requested */
    if (force_gui) {
	G_gui();
	return -1;
    }

    /* Split options where multiple answers are OK */
    split_opts();

    /* Check multiple options */
    error += check_multiple_opts();

    /* Check answers against options and check subroutines */
    if (!opt_checked)
	error += check_opts();

    /* Make sure all required options are set */
    error += check_required();


    if (error) {
	if (G_verbose() > G_verbose_min())
	    G_usage();
	return -1;
    }

    if (check_overwrite())
	return -1;

    return (0);
}


static int uses_new_gisprompt(void)
{
    struct Option *opt;
    char age[KEYLENGTH];
    char element[KEYLENGTH];
    char desc[KEYLENGTH];

    if (module_info.overwrite)
	return 1;

    /* figure out if any of the options use a "new" gisprompt */
    /* This is to see if we should spit out the --o flag      */
    if (n_opts) {
	opt = &first_option;
	while (opt != NULL) {
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


/**
 * \brief Command line help/usage message.
 *
 * Calls to <i>G_usage()</i> allow the programmer to print the usage 
 * message at any time. This will explain the allowed and required 
 * command line input to the user. This description is given according 
 * to the programmer's definitions for options and flags. This function 
 * becomes useful when the user enters options and/or flags on the 
 * command line that are syntactically valid to the parser, but 
 * functionally invalid for the command (e.g. an invalid file name.)<br>
 * For example, the parser logic doesn't directly support grouping 
 * options. If two options be specified together or not at all, the 
 * parser must be told that these options are not required and the 
 * programmer must check that if one is specified the other must be as 
 * well. If this additional check fails, then <i>G_parser()</i> will 
 * succeed, but the programmer can then call <i>G_usage()</i>  to print 
 * the standard usage message and print additional information about how 
 * the two options work together.
 *
 * \return always returns 0
 */

int G_usage(void)
{
    struct Option *opt;
    struct Flag *flag;
    char item[256];
    const char *key_desc;
    int maxlen;
    int len, n;
    int new_prompt = 0;

    new_prompt = uses_new_gisprompt();

    if (!pgm_name)		/* v.dave && r.michael */
	pgm_name = G_program_name();
    if (!pgm_name)
	pgm_name = "??";

    if (module_info.label || module_info.description) {
	fprintf(stderr, _("\nDescription:\n"));
	if (module_info.label)
	    fprintf(stderr, " %s\n", module_info.label);
	if (module_info.description)
	    fprintf(stderr, " %s\n", module_info.description);
    }
    if (module_info.keywords) {
	fprintf(stderr, _("\nKeywords:\n"));
	fprintf(stderr, " %s\n", module_info.keywords);
    }

    fprintf(stderr, _("\nUsage:\n "));

    len = show(pgm_name, 1);

    /* Print flags */

    if (n_flags) {
	item[0] = ' ';
	item[1] = '[';
	item[2] = '-';
	flag = &first_flag;
	for (n = 3; flag != NULL; n++, flag = flag->next_flag)
	    item[n] = flag->key;
	item[n++] = ']';
	item[n] = 0;
	len = show(item, len);
    }

    maxlen = 0;
    if (n_opts) {
	opt = &first_option;
	while (opt != NULL) {
	    if (opt->key_desc != NULL)
		key_desc = opt->key_desc;
	    else if (opt->type == TYPE_STRING)
		key_desc = "string";
	    else
		key_desc = "value";

	    n = strlen(opt->key);
	    if (n > maxlen)
		maxlen = n;

	    strcpy(item, " ");
	    if (!opt->required)
		strcat(item, "[");
	    strcat(item, opt->key);
	    strcat(item, "=");
	    strcat(item, key_desc);
	    if (opt->multiple) {
		strcat(item, "[,");
		strcat(item, key_desc);
		strcat(item, ",...]");
	    }
	    if (!opt->required)
		strcat(item, "]");

	    len = show(item, len);

	    opt = opt->next_opt;
	}
    }
    if (new_prompt) {
	strcpy(item, " [--overwrite]");
	len = show(item, len);
    }

    strcpy(item, " [--verbose]");
    len = show(item, len);

    strcpy(item, " [--quiet]");
    len = show(item, len);


    fprintf(stderr, "\n");

    /* Print help info for flags */

    fprintf(stderr, _("\nFlags:\n"));

    if (n_flags) {
	flag = &first_flag;
	while (flag != NULL) {
	    fprintf(stderr, "  -%c   ", flag->key);

	    if (flag->label) {
		fprintf(stderr, "%s\n", flag->label);
		if (flag->description)
		    fprintf(stderr, "        %s\n", flag->description);

	    }
	    else if (flag->description) {
		fprintf(stderr, "%s\n", flag->description);
	    }

	    flag = flag->next_flag;
	}
    }

    if (new_prompt)
	fprintf(stderr, " --o   %s\n",
		_("Allow output files to overwrite existing files"));

    fprintf(stderr, " --v   %s\n", _("Verbose module output"));
    fprintf(stderr, " --q   %s\n", _("Quiet module output"));

    /* Print help info for options */

    if (n_opts) {
	fprintf(stderr, _("\nParameters:\n"));
	opt = &first_option;
	while (opt != NULL) {
	    fprintf(stderr, "  %*s   ", maxlen, opt->key);

	    if (opt->label) {
		fprintf(stderr, "%s\n", opt->label);
		if (opt->description) {
		    fprintf(stderr, "  %*s    %s\n",
			    maxlen, " ", opt->description);
		}
	    }
	    else if (opt->description) {
		fprintf(stderr, "%s\n", opt->description);
	    }

	    if (opt->options)
		show_options(maxlen, opt->options);
	    /*
	       fprintf (stderr, "  %*s   options: %s\n", maxlen, " ",
	       _(opt->options)) ;
	     */
	    if (opt->def)
		fprintf(stderr, _("  %*s   default: %s\n"), maxlen, " ",
			opt->def);

	    if (opt->descs) {
		int i = 0;

		while (opt->opts[i]) {
		    if (opt->descs[i])
			fprintf(stderr, "  %*s    %s: %s\n",
				maxlen, " ", opt->opts[i], opt->descs[i]);

		    i++;
		}
	    }

	    opt = opt->next_opt;
	}
    }

    return 0;
}


/**
 * \brief Formats text for XML.
 *
 * \param[in,out] fp file to write to
 * \param[in] str string to write
 */

static void print_escaped_for_xml(FILE * fp, const char *str)
{
    for (; *str; str++) {
	switch (*str) {
	case '&':
	    fputs("&amp;", fp);
	    break;
	case '<':
	    fputs("&lt;", fp);
	    break;
	case '>':
	    fputs("&gt;", fp);
	    break;
	default:
	    fputc(*str, fp);
	}
    }
}


/**
 * \brief Format text for HTML output
 */
#define do_escape(c,escaped) case c: fputs(escaped,f);break
static void print_escaped_for_html(FILE * f, const char *str)
{
    const char *s;

    for (s = str; *s; s++) {
	switch (*s) {
	    do_escape('&', "&amp;");
	    do_escape('<', "&lt;");
	    do_escape('>', "&gt;");
	    do_escape('\n', "<br>");
	default:
	    fputc(*s, f);
	}
    }
}

#undef do_escape

/**
   \brief Print module usage description in XML format.
**/
static void G_usage_xml(void)
{
    struct Option *opt;
    struct Flag *flag;
    char *type;
    char *s, *top;
    int i;
    char *encoding;
    int new_prompt = 0;

    new_prompt = uses_new_gisprompt();

    /* gettext converts strings to encoding returned by nl_langinfo(CODESET) */

#if defined(HAVE_LANGINFO_H)
    encoding = nl_langinfo(CODESET);
    if (!encoding || strlen(encoding) == 0) {
	encoding = "UTF-8";
    }
#else
    encoding = "UTF-8";
#endif

    if (!pgm_name)		/* v.dave && r.michael */
	pgm_name = G_program_name();
    if (!pgm_name)
	pgm_name = "??";

    fprintf(stdout, "<?xml version=\"1.0\" encoding=\"%s\"?>\n", encoding);
    fprintf(stdout, "<!DOCTYPE task SYSTEM \"grass-interface.dtd\">\n");

    fprintf(stdout, "<task name=\"%s\">\n", pgm_name);

    if (module_info.label) {
	fprintf(stdout, "\t<label>\n\t\t");
	print_escaped_for_xml(stdout, module_info.label);
	fprintf(stdout, "\n\t</label>\n");
    }

    if (module_info.description) {
	fprintf(stdout, "\t<description>\n\t\t");
	print_escaped_for_xml(stdout, module_info.description);
	fprintf(stdout, "\n\t</description>\n");
    }

    if (module_info.keywords) {
	fprintf(stdout, "\t<keywords>\n\t\t");
	print_escaped_for_xml(stdout, module_info.keywords);
	fprintf(stdout, "\n\t</keywords>\n");
    }

	/***** Don't use parameter-groups for now.  We'll reimplement this later 
	 ***** when we have a concept of several mutually exclusive option
	 ***** groups
	if (n_opts || n_flags)
		fprintf(stdout, "\t<parameter-group>\n");
	 *****
	 *****
	 *****/

    if (n_opts) {
	opt = &first_option;
	while (opt != NULL) {
	    /* TODO: make this a enumeration type? */
	    switch (opt->type) {
	    case TYPE_INTEGER:
		type = "integer";
		break;
	    case TYPE_DOUBLE:
		type = "float";
		break;
	    case TYPE_STRING:
		type = "string";
		break;
	    default:
		type = "string";
		break;
	    }
	    fprintf(stdout, "\t<parameter "
		    "name=\"%s\" "
		    "type=\"%s\" "
		    "required=\"%s\" "
		    "multiple=\"%s\">\n",
		    opt->key,
		    type,
		    opt->required == YES ? "yes" : "no",
		    opt->multiple == YES ? "yes" : "no");

	    if (opt->label) {
		fprintf(stdout, "\t\t<label>\n\t\t\t");
		print_escaped_for_xml(stdout, opt->label);
		fprintf(stdout, "\n\t\t</label>\n");
	    }

	    if (opt->description) {
		fprintf(stdout, "\t\t<description>\n\t\t\t");
		print_escaped_for_xml(stdout, opt->description);
		fprintf(stdout, "\n\t\t</description>\n");
	    }

	    if (opt->key_desc) {
		fprintf(stdout, "\t\t<keydesc>\n");
		top = G_calloc(strlen(opt->key_desc) + 1, 1);
		strcpy(top, opt->key_desc);
		s = strtok(top, ",");
		for (i = 1; s != NULL; i++) {
		    fprintf(stdout, "\t\t\t<item order=\"%d\">", i);
		    print_escaped_for_xml(stdout, s);
		    fprintf(stdout, "</item>\n");
		    s = strtok(NULL, ",");
		}
		fprintf(stdout, "\t\t</keydesc>\n");
		G_free(top);
	    }

	    if (opt->gisprompt) {
		const char *atts[] = { "age", "element", "prompt", NULL };
		top = G_calloc(strlen(opt->gisprompt) + 1, 1);
		strcpy(top, opt->gisprompt);
		s = strtok(top, ",");
		fprintf(stdout, "\t\t<gisprompt ");
		for (i = 0; s != NULL && atts[i] != NULL; i++) {
		    fprintf(stdout, "%s=\"%s\" ", atts[i], s);
		    s = strtok(NULL, ",");
		}
		fprintf(stdout, "/>\n");
		G_free(top);
	    }

	    if (opt->def) {
		fprintf(stdout, "\t\t<default>\n\t\t\t");
		print_escaped_for_xml(stdout, opt->def);
		fprintf(stdout, "\n\t\t</default>\n");
	    }

	    if (opt->options) {
		/* TODO:
		 * add something like
		 *       <range min="xxx" max="xxx"/>
		 * to <values> */
		i = 0;
		fprintf(stdout, "\t\t<values>\n");
		while (opt->opts[i]) {
		    fprintf(stdout, "\t\t\t<value>\n");
		    fprintf(stdout, "\t\t\t\t<name>");
		    print_escaped_for_xml(stdout, opt->opts[i]);
		    fprintf(stdout, "</name>\n");
		    if (opt->descs && opt->opts[i] && opt->descs[i]) {
			fprintf(stdout, "\t\t\t\t<description>");
			print_escaped_for_xml(stdout, opt->descs[i]);
			fprintf(stdout, "</description>\n");
		    }
		    fprintf(stdout, "\t\t\t</value>\n");
		    i++;
		}
		fprintf(stdout, "\t\t</values>\n");
	    }
	    if (opt->guisection) {
		fprintf(stdout, "\t\t<guisection>\n\t\t\t");
		print_escaped_for_xml(stdout, opt->guisection);
		fprintf(stdout, "\n\t\t</guisection>\n");
	    }
	    /* TODO:
	     * - key_desc?
	     * - there surely are some more. which ones?
	     */

	    opt = opt->next_opt;
	    fprintf(stdout, "\t</parameter>\n");
	}
    }


    if (n_flags) {
	flag = &first_flag;
	while (flag != NULL) {
	    fprintf(stdout, "\t<flag name=\"%c\">\n", flag->key);

	    if (flag->label) {
		fprintf(stdout, "\t\t<label>\n\t\t\t");
		print_escaped_for_xml(stdout, flag->label);
		fprintf(stdout, "\n\t\t</label>\n");
	    }

	    if (flag->description) {
		fprintf(stdout, "\t\t<description>\n\t\t\t");
		print_escaped_for_xml(stdout, flag->description);
		fprintf(stdout, "\n\t\t</description>\n");
	    }
	    if (flag->guisection) {
		fprintf(stdout, " \t\t<guisection>\n\t\t\t");
		print_escaped_for_xml(stdout, flag->guisection);
		fprintf(stdout, "\n\t\t</guisection>\n");
	    }
	    flag = flag->next_flag;
	    fprintf(stdout, "\t</flag>\n");
	}
    }

	/***** Don't use parameter-groups for now.  We'll reimplement this later 
	 ***** when we have a concept of several mutually exclusive option
	 ***** groups
	if (n_opts || n_flags)
		fprintf(stdout, "\t</parameter-group>\n");
	 *****
	 *****
	 *****/

    if (new_prompt) {
	/* overwrite */
	fprintf(stdout, "\t<flag name=\"%s\">\n", "overwrite");
	fprintf(stdout, "\t\t<description>\n\t\t\t");
	print_escaped_for_xml(stdout,
			      "Allow output files to overwrite existing files");
	fprintf(stdout, "\n\t\t</description>\n");
	fprintf(stdout, "\t</flag>\n");
    }

    /* verbose */
    fprintf(stdout, "\t<flag name=\"%s\">\n", "verbose");
    fprintf(stdout, "\t\t<description>\n\t\t\t");
    print_escaped_for_xml(stdout, "Verbose module output");
    fprintf(stdout, "\n\t\t</description>\n");
    fprintf(stdout, "\t</flag>\n");

    /* quiet */
    fprintf(stdout, "\t<flag name=\"%s\">\n", "quiet");
    fprintf(stdout, "\t\t<description>\n\t\t\t");
    print_escaped_for_xml(stdout, "Quiet module output");
    fprintf(stdout, "\n\t\t</description>\n");
    fprintf(stdout, "\t</flag>\n");

    fprintf(stdout, "</task>\n");
}

/**
   \brief Print module usage description in HTML format.
**/
static void G_usage_html(void)
{
    struct Option *opt;
    struct Flag *flag;
    const char *type;
    int new_prompt = 0;

    new_prompt = uses_new_gisprompt();

    if (!pgm_name)		/* v.dave && r.michael */
	pgm_name = G_program_name();
    if (!pgm_name)
	pgm_name = "??";

    fprintf(stdout,
	    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
    fprintf(stdout, "<html>\n<head>\n");
    fprintf(stdout, "<title>GRASS GIS: %s</title>\n", pgm_name);
    fprintf(stdout,
	    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n");
    fprintf(stdout,
	    "<link rel=\"stylesheet\" href=\"grassdocs.css\" type=\"text/css\">\n");
    fprintf(stdout, "</head>\n");
    fprintf(stdout, "<body bgcolor=\"white\">\n\n");
    fprintf(stdout,
	    "<img src=\"grass_logo.png\" alt=\"GRASS logo\"><hr align=center size=6 noshade>\n\n");
    fprintf(stdout, "<h2>%s</h2>\n", _("NAME"));
    fprintf(stdout, "<em><b>%s</b></em> ", pgm_name);

    if (module_info.label || module_info.description)
	fprintf(stdout, " - ");

    if (module_info.label)
	fprintf(stdout, "%s<BR>\n", module_info.label);

    if (module_info.description)
	fprintf(stdout, "%s\n", module_info.description);


    fprintf(stdout, "<h2>%s</h2>\n", _("KEYWORDS"));
    if (module_info.keywords) {
	fprintf(stdout, "%s", module_info.keywords);
	fprintf(stdout, "\n");
    }
    fprintf(stdout, "<h2>%s</h2>\n", _("SYNOPSIS"));
    fprintf(stdout, "<b>%s</b><br>\n", pgm_name);
    fprintf(stdout, "<b>%s help</b><br>\n", pgm_name);

    fprintf(stdout, "<b>%s</b>", pgm_name);



    /* print short version first */
    if (n_flags) {
	flag = &first_flag;
	fprintf(stdout, " [-<b>");
	while (flag != NULL) {
	    fprintf(stdout, "%c", flag->key);
	    flag = flag->next_flag;
	}
	fprintf(stdout, "</b>] ");
    }
    else
	fprintf(stdout, " ");

    if (n_opts) {
	opt = &first_option;

	while (opt != NULL) {
	    if (opt->key_desc != NULL)
		type = opt->key_desc;
	    else
		switch (opt->type) {
		case TYPE_INTEGER:
		    type = "integer";
		    break;
		case TYPE_DOUBLE:
		    type = "float";
		    break;
		case TYPE_STRING:
		    type = "string";
		    break;
		default:
		    type = "string";
		    break;
		}
	    if (!opt->required)
		fprintf(stdout, " [");
	    fprintf(stdout, "<b>%s</b>=<em>%s</em>", opt->key, type);
	    if (opt->multiple) {
		fprintf(stdout, "[,<i>%s</i>,...]", type);
	    }
	    if (!opt->required)
		fprintf(stdout, "] ");

	    opt = opt->next_opt;
	    fprintf(stdout, " ");
	}
    }
    if (new_prompt)
	fprintf(stdout, " [--<b>overwrite</b>] ");

    fprintf(stdout, " [--<b>verbose</b>] ");
    fprintf(stdout, " [--<b>quiet</b>] ");

    fprintf(stdout, "\n");


    /* now long version */
    fprintf(stdout, "\n");
    if (n_flags || new_prompt) {
	flag = &first_flag;
	fprintf(stdout, "<h3>%s:</h3>\n", _("Flags"));
	fprintf(stdout, "<DL>\n");
	while (n_flags && flag != NULL) {
	    fprintf(stdout, "<DT><b>-%c</b></DT>\n", flag->key);

	    if (flag->label) {
		fprintf(stdout, "<DD>");
		fprintf(stdout, "%s", flag->label);
		fprintf(stdout, "</DD>\n");
	    }

	    if (flag->description) {
		fprintf(stdout, "<DD>");
		fprintf(stdout, "%s", flag->description);
		fprintf(stdout, "</DD>\n");
	    }

	    flag = flag->next_flag;
	    fprintf(stdout, "\n");
	}
	if (new_prompt) {
	    fprintf(stdout, "<DT><b>--overwrite</b></DT>\n");
	    fprintf(stdout, "<DD>%s</DD>\n",
		    _("Allow output files to overwrite existing files"));
	}

	fprintf(stdout, "<DT><b>--verbose</b></DT>\n");
	fprintf(stdout, "<DD>%s</DD>\n", _("Verbose module output"));

	fprintf(stdout, "<DT><b>--quiet</b></DT>\n");
	fprintf(stdout, "<DD>%s</DD>\n", _("Quiet module output"));

	fprintf(stdout, "</DL>\n");
    }

    fprintf(stdout, "\n");
    if (n_opts) {
	opt = &first_option;
	fprintf(stdout, "<h3>%s:</h3>\n", _("Parameters"));
	fprintf(stdout, "<DL>\n");

	while (opt != NULL) {
	    /* TODO: make this a enumeration type? */
	    if (opt->key_desc != NULL)
		type = opt->key_desc;
	    else
		switch (opt->type) {
		case TYPE_INTEGER:
		    type = "integer";
		    break;
		case TYPE_DOUBLE:
		    type = "float";
		    break;
		case TYPE_STRING:
		    type = "string";
		    break;
		default:
		    type = "string";
		    break;
		}
	    fprintf(stdout, "<DT><b>%s</b>=<em>%s", opt->key, type);
	    if (opt->multiple) {
		fprintf(stdout, "[,<i>%s</i>,...]", type);
	    }
	    fprintf(stdout, "</em></DT>\n");

	    if (opt->label) {
		fprintf(stdout, "<DD>");
		fprintf(stdout, "%s", opt->label);
		fprintf(stdout, "</DD>\n");
	    }
	    if (opt->description) {
		fprintf(stdout, "<DD>");
		print_escaped_for_html(stdout, opt->description);
		fprintf(stdout, "</DD>\n");
	    }

	    if (opt->options) {
		fprintf(stdout, "<DD>%s: <em>", _("Options"));
		fprintf(stdout, "%s", opt->options);
		fprintf(stdout, "</em></DD>\n");
	    }

	    if (opt->def) {
		fprintf(stdout, "<DD>%s: <em>", _("Default"));
		fprintf(stdout, "%s", opt->def);
		fprintf(stdout, "</em></DD>\n");
	    }

	    if (opt->descs) {
		int i = 0;

		while (opt->opts[i]) {
		    if (opt->descs[i])
			fprintf(stdout, "<DD><b>%s</b>: %s</DD>\n",
				opt->opts[i], opt->descs[i]);
		    i++;
		}
	    }

	    opt = opt->next_opt;
	    fprintf(stdout, "\n");
	}
	fprintf(stdout, "</DL>\n");
    }

    fprintf(stdout, "</body>\n</html>\n");
}

/**
   \brief Print a module parameter template to assist with creating shell script wrappers.
**/
static void G_script(void)
{
    FILE *fp = stdout;
    char *type;

    fprintf(fp, "#!/bin/sh\n\n");
    fprintf(fp,
	    "############################################################################\n");
    fprintf(fp, "#\n");
    fprintf(fp, "# MODULE:       %s_wrapper\n", G_program_name());
    fprintf(fp, "# AUTHOR(S):    %s\n", G_whoami());
    fprintf(fp, "# PURPOSE:      \n");
    fprintf(fp, "# COPYRIGHT:    (C) 2008 GRASS Development Team/%s\n",
	    G_whoami());
    fprintf(fp, "#\n");
    fprintf(fp,
	    "#  This program is free software; you can redistribute it and/or modify\n");
    fprintf(fp,
	    "#  it under the terms of the GNU General Public License as published by\n");
    fprintf(fp,
	    "#  the Free Software Foundation; either version 2 of the License, or\n");
    fprintf(fp, "#  (at your option) any later version.\n");
    fprintf(fp, "#\n");
    fprintf(fp,
	    "#  This program is distributed in the hope that it will be useful,\n");
    fprintf(fp,
	    "#  but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    fprintf(fp,
	    "#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    fprintf(fp, "#  GNU General Public License for more details.\n");
    fprintf(fp, "#\n");
    fprintf(fp,
	    "#############################################################################/\n");

    fprintf(fp, "#%%Module\n");
    if (module_info.label)
	fprintf(fp, "#%% label: %s\n", module_info.label);
    if (module_info.description)
	fprintf(fp, "#%% description: %s\n", module_info.description);
    if (module_info.keywords)
	fprintf(fp, "#%% keywords: %s\n", module_info.keywords);
    fprintf(fp, "#%%End\n");

    if (n_flags) {
	struct Flag *flag;

	for (flag = &first_flag; flag; flag = flag->next_flag) {
	    fprintf(fp, "#%%Flag\n");
	    fprintf(fp, "#%% key: %c\n", flag->key);
	    if (flag->label)
		fprintf(fp, "#%% label: %s\n", flag->label);
	    if (flag->description)
		fprintf(fp, "#%% description: %s\n", flag->description);
	    if (flag->guisection)
		fprintf(fp, "#%% guisection: %s\n", flag->guisection);
	    fprintf(fp, "#%%End\n");
	}
    }

    if (n_opts) {
	struct Option *opt;

	for (opt = &first_option; opt; opt = opt->next_opt) {
	    switch (opt->type) {
	    case TYPE_INTEGER:
		type = "integer";
		break;
	    case TYPE_DOUBLE:
		type = "double";
		break;
	    case TYPE_STRING:
		type = "string";
		break;
	    default:
		type = "string";
		break;
	    }

	    fprintf(fp, "#%%Option\n");
	    fprintf(fp, "#%% key: %s\n", opt->key);
	    fprintf(fp, "#%% type: %s\n", type);
	    fprintf(fp, "#%% required: %s\n", opt->required ? "yes" : "no");
	    fprintf(fp, "#%% multiple: %s\n", opt->multiple ? "yes" : "no");
	    if (opt->options)
		fprintf(fp, "#%% options: %s\n", opt->options);
	    if (opt->key_desc)
		fprintf(fp, "#%% key_desc: %s\n", opt->key_desc);
	    if (opt->label)
		fprintf(fp, "#%% label: %s\n", opt->label);
	    if (opt->description)
		fprintf(fp, "#%% description: %s\n", opt->description);
	    if (opt->descriptions)
		fprintf(fp, "#%% descriptions: %s\n", opt->descriptions);
	    if (opt->answer)
		fprintf(fp, "#%% answer: %s\n", opt->answer);
	    if (opt->gisprompt)
		fprintf(fp, "#%% gisprompt: %s\n", opt->gisprompt);
	    if (opt->guisection)
		fprintf(fp, "#%% guisection: %s\n", opt->guisection);
	    fprintf(fp, "#%%End\n");
	}
    }

    fprintf(fp,
	    "\nif [ -z \"$GISBASE\" ] ; then\n"
	    "    echo \"You must be in GRASS GIS to run this program.\" 1>&2\n"
	    "    exit 1\n"
	    "fi\n"
	    "\n"
	    "if [ \"$1\" != \"@ARGS_PARSED@\" ] ; then\n"
	    "    exec g.parser \"$0\" \"$@\"\n"
	    "fi\n" "\n" "# CODE GOES HERE\n" "\n");
}

/**
  \brief Generates tcltk dialog.

  \param[out] fp File to store tcltk code
**/
static void generate_tcl(FILE * fp)
{
    int new_prompt = uses_new_gisprompt();
    const char *type;
    int optn;

    fprintf(fp, "begin_dialog {%s} {\n", pgm_name);
    fprintf(fp, " label {%s}\n", module_info.label ? module_info.label : "");
    fprintf(fp, " desc {%s}\n",
	    module_info.description ? module_info.description : "");
    fprintf(fp, " key {%s}\n",
	    module_info.keywords ? module_info.keywords : "");
    fprintf(fp, "}\n");

    optn = 1;

    if (n_flags) {
	struct Flag *flag;

	for (flag = &first_flag; flag; flag = flag->next_flag, optn++) {
	    fprintf(fp, "add_flag %d {\n", optn);
	    fprintf(fp, " name {%c}\n", flag->key);
	    fprintf(fp, " desc {%s}\n", flag->description);
	    fprintf(fp, " answer %d\n", flag->answer);
	    /* It should be up to the gui as to what
	       to do with the label and description */
	    fprintf(fp, " label {%s}\n", flag->label ? flag->label : "");
	    fprintf(fp, " guisection {%s}\n",
		    flag->guisection ? flag->guisection : "");
	    fprintf(fp, "}\n");
	}
    }

    if (n_opts) {
	struct Option *opt;

	for (opt = &first_option; opt; opt = opt->next_opt, optn++) {
	    if (opt->key_desc != NULL)
		type = opt->key_desc;
	    else
		switch (opt->type) {
		case TYPE_INTEGER:
		    type = "integer";
		    break;
		case TYPE_DOUBLE:
		    type = "float";
		    break;
		case TYPE_STRING:
		    type = "string";
		    break;
		default:
		    type = "string";
		    break;
		}

	    fprintf(fp, "add_option %d {\n", optn);
	    fprintf(fp, " name {%s}\n", opt->key);
	    fprintf(fp, " type {%s}\n", type);
	    fprintf(fp, " multi %d\n", opt->multiple);
	    fprintf(fp, " desc {%s}\n", opt->description);
	    fprintf(fp, " required %d\n", opt->required);
	    fprintf(fp, " options {%s}\n", opt->options ? opt->options : "");
	    fprintf(fp, " descs {%s}\n",
		    opt->descriptions ? opt->descriptions : "");
	    fprintf(fp, " answer {%s}\n", opt->answer ? opt->answer : "");
	    fprintf(fp, " prompt {%s}\n",
		    opt->gisprompt ? opt->gisprompt : "");
	    /* It should be up to the gui as to what
	       to do with the label and description */
	    fprintf(fp, " label {%s}\n", opt->label ? opt->label : "");
	    fprintf(fp, " guisection {%s}\n",
		    opt->guisection ? opt->guisection : "");
	    fprintf(fp, "}\n");
	}
    }

    if (new_prompt) {
	fprintf(fp, "add_xflag %d {\n", optn);
	fprintf(fp, " name {overwrite}\n");
	fprintf(fp, " desc {%s}\n",
		_("Allow output files to overwrite existing files"));
	fprintf(fp, " answer %d\n", overwrite);
	fprintf(fp, " label {%s}\n", _("Allow overwrite"));
	fprintf(fp, " guisection {}\n");
	fprintf(fp, "}\n");
	optn++;
    }

    fprintf(fp, "add_xflag %d {\n", optn);
    fprintf(fp, " name {quiet}\n");
    fprintf(fp, " desc {%s}\n", _("Run with minimal output messages"));
    fprintf(fp, " answer %d\n", quiet);
    fprintf(fp, " label {%s}\n", _("Run quietly"));
    fprintf(fp, " guisection {}\n");
    fprintf(fp, "}\n");
    optn++;

    fprintf(fp, "end_dialog %d\n", optn - 1);
}

/**
   \brief Build Tcl/Tk GUI dialog
**/
static void G_gui_tcltk(void)
{
    FILE *fp;

    if (!pgm_name)
	pgm_name = G_program_name();
    if (!pgm_name)
	pgm_name = "??";

#ifdef __MINGW32__
    if (getenv("GRASS_DEBUG_GUI"))
	fp = G_popen("tee gui_dump.tcl | %GRASS_WISH%", "w");
    else
	fp = G_popen("%GRASS_WISH%", "w");
#else
    if (getenv("GRASS_DEBUG_GUI"))
	fp = G_popen("tee gui_dump.tcl | $GRASS_WISH", "w");
    else
	fp = G_popen("$GRASS_WISH", "w");
#endif

    if (!fp)
	G_fatal_error(_("Unable to spawn the 'wish' program"));

    fprintf(fp, "source $env(GISBASE)/etc/gui.tcl\n");

    generate_tcl(fp);

    G_pclose(fp);
}

/**
   \brief Build wxPython GUI dialog
**/
static void G_gui_wx(void)
{
    char script[GPATH_MAX];

    if (!pgm_name)
	pgm_name = G_program_name();
    if (!pgm_name)
	G_fatal_error(_("Unable to determine program name"));

    sprintf(script, "%s/etc/wxpython/gui_modules/menuform.py",
	    getenv("GISBASE"));
    G_spawn("python", "menuform.py", script, pgm_name, NULL);
}

/**
   \brief Invoke GUI dialog 

   Use G_gui_wx() or G_gui_tcltk() to generate GUI dialog.

   G_gui_tcltk() is called by default (if GRASS_GUI is not defined)
**/
static void G_gui(void)
{
    /* read environment variables first then internal GRASS variable */
    char *gui = getenv("GRASS_GUI");

    if (!gui) {
	gui = G_getenv("GRASS_GUI");
    }

    if (gui && strcmp(gui, "wxpython") == 0)
	G_gui_wx();
    else
	G_gui_tcltk();

    return;
}

/**
   \brief Send Tcl/Tk code to tcltkgrass 
**/
static void G_tcltk(void)
{
    if (!pgm_name)
	pgm_name = G_program_name();
    if (!pgm_name)
	pgm_name = "??";

    generate_tcl(stdout);
}

/**************************************************************************
 *
 * The remaining routines are all local (static) routines used to support
 * the parsing process.
 *
 **************************************************************************/

static int show_options(int maxlen, const char *str)
{
    char *buff = G_store(str);
    char *p1, *p2;
    int totlen, len;

    fprintf(stderr, _("  %*s   options: "), maxlen, " ");
    totlen = maxlen + 13;
    p1 = buff;
    while ((p2 = G_index(p1, ','))) {
	*p2 = '\0';
	len = strlen(p1) + 1;
	if ((len + totlen) > 76) {
	    totlen = maxlen + 13;
	    fprintf(stderr, "\n %*s", maxlen + 13, " ");
	}
	fprintf(stderr, "%s,", p1);
	totlen += len;
	p1 = p2 + 1;
    }
    len = strlen(p1);
    if ((len + totlen) > 76)
	fprintf(stderr, "\n %*s", maxlen + 13, " ");
    fprintf(stderr, "%s\n", p1);

    G_free(buff);

    return 0;
}

static int show(const char *item, int len)
{
    int n;

    n = strlen(item) + (len > 0);
    if (n + len > 76) {
	if (len)
	    fprintf(stderr, "\n  ");
	len = 0;
    }
    fprintf(stderr, "%s", item);
    return n + len;
}

static int set_flag(int f)
{
    struct Flag *flag;

    /* Flag is not valid if there are no flags to set */

    if (!n_flags) {
	fprintf(stderr, _("Sorry, <%c> is not a valid flag\n"), f);
	return (1);
    }

    /* Find flag with corrrect keyword */

    flag = &first_flag;
    while (flag != NULL) {
	if (flag->key == f) {
	    flag->answer = 1;
	    return (0);
	}
	flag = flag->next_flag;
    }

    fprintf(stderr, _("Sorry, <%c> is not a valid flag\n"), f);
    return (1);
}

/* contents() is used to find things strings with characters like commas and
 * dashes.
 */
static int contains(const char *s, int c)
{
    while (*s) {
	if (*s == c)
	    return (1);
	s++;
    }
    return (0);
}

static int is_option(const char *string)
{
    const char *p = strchr(string, '=');

    if (!p)
	return 0;
    if (p == string)
	return 0;
    p--;
    if (!strchr("abcdefghijklmnopqrstuvwxyz0123456789", *p))
	return 0;

    return 1;
}

static int set_option(char *string)
{
    struct Option *at_opt = NULL;
    struct Option *opt = NULL;
    int got_one;
    size_t key_len;
    char the_key[KEYLENGTH];
    char *ptr;

    for (ptr = the_key; *string != '='; ptr++, string++)
	*ptr = *string;
    *ptr = '\0';
    string++;

    /* Find option with best keyword match */
    got_one = 0;
    key_len = strlen(the_key);
    for (at_opt = &first_option; at_opt != NULL; at_opt = at_opt->next_opt) {
	if (at_opt->key == NULL || strncmp(the_key, at_opt->key, key_len))
	    continue;

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
	fprintf(stderr, _("Sorry, <%s=> is ambiguous\n"), the_key);
	return (1);
    }

    /* If there is no match, complain */
    if (got_one == 0) {
	fprintf(stderr, _("Sorry, <%s> is not a valid parameter\n"), the_key);
	return (1);
    }

    /* Allocate memory where answer is stored */
    if (opt->count++) {
	opt->answer = (char *)G_realloc(opt->answer,
					strlen(opt->answer) + strlen(string) +
					2);
	strcat(opt->answer, ",");
	strcat(opt->answer, string);
    }
    else
	opt->answer = G_store(string);
    return (0);
}

static int check_opts(void)
{
    struct Option *opt;
    int error;
    int ans;

    error = 0;

    if (!n_opts)
	return (0);

    opt = &first_option;
    while (opt != NULL) {
	/* Check answer against options if any */

	if (opt->options && opt->answer) {
	    if (opt->multiple == 0)
		error += check_an_opt(opt->key, opt->type,
				      opt->options, opt->answer);
	    else {
		for (ans = 0; opt->answers[ans] != '\0'; ans++)
		    error += check_an_opt(opt->key, opt->type,
					  opt->options, opt->answers[ans]);
	    }
	}

	/* Check answer against user's check subroutine if any */

	if (opt->checker)
	    error += opt->checker(opt->answer);

	opt = opt->next_opt;
    }
    return (error);
}

static int check_an_opt(const char *key, int type, const char *options,
			const char *answer)
{
    int error;

    error = 0;

    switch (type) {
    case TYPE_INTEGER:
	error = check_int(answer, options);
	break;
    case TYPE_DOUBLE:
	error = check_double(answer, options);
	break;
    case TYPE_STRING:
	error = check_string(answer, options);
	break;
	/*
	   case TYPE_COORDINATE:
	   error = check_coor(answer,options) ;
	   break ;
	 */
    }
    switch (error) {
    case 0:
	break;
    case BAD_SYNTAX:
	fprintf(stderr,
		_("\nERROR: illegal range syntax for parameter <%s>\n"), key);
	fprintf(stderr, _("       Presented as: %s\n"), options);
	break;
    case OUT_OF_RANGE:
	fprintf(stderr,
		_("\nERROR: value <%s> out of range for parameter <%s>\n"),
		answer, key);
	fprintf(stderr, _("       Legal range: %s\n"), options);
	break;
    case MISSING_VALUE:
	fprintf(stderr, _("\nERROR: Missing value for parameter <%s>\n"),
		key);
    }
    return (error);
}

static int check_int(const char *ans, const char *opts)
{
    int d, lo, hi;

    if (1 != sscanf(ans, "%d", &d))
	return (MISSING_VALUE);

    if (contains(opts, '-')) {
	if (2 != sscanf(opts, "%d-%d", &lo, &hi))
	    return (BAD_SYNTAX);
	if (d < lo || d > hi)
	    return (OUT_OF_RANGE);
	else
	    return (0);
    }
    else if (contains(opts, ',')) {
	for (;;) {
	    if (1 != sscanf(opts, "%d", &lo))
		return (BAD_SYNTAX);
	    if (d == lo)
		return (0);
	    while (*opts != '\0' && *opts != ',')
		opts++;
	    if (*opts == '\0')
		return (OUT_OF_RANGE);
	    if (*(++opts) == '\0')
		return (OUT_OF_RANGE);
	}
    }
    else {
	if (1 != sscanf(opts, "%d", &lo))
	    return (BAD_SYNTAX);
	if (d == lo)
	    return (0);
	return (OUT_OF_RANGE);
    }
}

/*
   static int
   check_coor(ans, opts)
   char *ans ;
   char *opts ;
   {
   double xd, xlo, xhi;
   double yd, ylo, yhi;

   if (1 != sscanf(ans,"%lf,%lf", &xd, &yd))
   return(MISSING_VALUE) ;

   if (contains(opts, '-'))
   {
   if (2 != sscanf(opts,"%lf-%lf,%lf-%lf",&xlo, &xhi, &ylo, &yhi))
   return(BAD_SYNTAX) ;
   if (xd < xlo || xd > xhi)
   return(OUT_OF_RANGE) ;
   if (yd < ylo || yd > yhi)
   return(OUT_OF_RANGE) ;
   return(0) ;
   }
   return(BAD_SYNTAX) ;
   }
 */

static int check_double(const char *ans, const char *opts)
{
    double d, lo, hi;

    if (1 != sscanf(ans, "%lf", &d))
	return (MISSING_VALUE);

    if (contains(opts, '-')) {
	if (2 != sscanf(opts, "%lf-%lf", &lo, &hi))
	    return (BAD_SYNTAX);
	if (d < lo || d > hi)
	    return (OUT_OF_RANGE);
	else
	    return (0);
    }
    else if (contains(opts, ',')) {
	for (;;) {
	    if (1 != sscanf(opts, "%lf", &lo))
		return (BAD_SYNTAX);
	    if (d == lo)
		return (0);
	    while (*opts != '\0' && *opts != ',')
		opts++;
	    if (*opts == '\0')
		return (OUT_OF_RANGE);
	    if (*(++opts) == '\0')
		return (OUT_OF_RANGE);
	}
    }
    else {
	if (1 != sscanf(opts, "%lf", &lo))
	    return (BAD_SYNTAX);
	if (d == lo)
	    return (0);
	return (OUT_OF_RANGE);
    }
}

static int check_string(const char *ans, const char *opts)
{
    if (*opts == '\0')
	return (0);

    if (contains(opts, ',')) {
	for (;;) {
	    if ((!strncmp(ans, opts, strlen(ans)))
		&& (*(opts + strlen(ans)) == ','
		    || *(opts + strlen(ans)) == '\0'))
		return (0);
	    while (*opts != '\0' && *opts != ',')
		opts++;
	    if (*opts == '\0')
		return (OUT_OF_RANGE);
	    if (*(++opts) == '\0')
		return (OUT_OF_RANGE);
	}
    }
    else {
	if (!strcmp(ans, opts))
	    return (0);
	return (OUT_OF_RANGE);
    }
}

static int check_required(void)
{
    struct Option *opt;
    int err;

    err = 0;

    if (!n_opts)
	return (0);

    opt = &first_option;
    while (opt != NULL) {
	if (opt->required && opt->answer == NULL) {
	    fprintf(stderr,
		    _
		    ("\nERROR: Required parameter <%s> not set:\n    (%s).\n"),
		    opt->key, opt->description);
	    err++;
	}
	opt = opt->next_opt;
    }

    return (err);
}

static int split_opts(void)
{
    struct Option *opt;
    char *ptr1;
    char *ptr2;
    int allocated;
    int ans_num;
    int len;


    if (!n_opts)
	return 0;

    opt = &first_option;
    while (opt != NULL) {
	if ( /*opt->multiple && */ (opt->answer != NULL)) {
	    /* Allocate some memory to store array of pointers */
	    allocated = 10;
	    opt->answers = (char **)G_malloc(allocated * sizeof(char *));

	    ans_num = 0;
	    ptr1 = opt->answer;
	    opt->answers[ans_num] = NULL;

	    for (;;) {
		for (len = 0, ptr2 = ptr1; *ptr2 != '\0' && *ptr2 != ',';
		     ptr2++, len++) ;

		if (len > 0) {	/* skip ,, */
		    opt->answers[ans_num] = (char *)G_malloc(len + 1);
		    G_copy(opt->answers[ans_num], ptr1, len);
		    opt->answers[ans_num][len] = 0;

		    ans_num++;

		    if (ans_num >= allocated) {
			allocated += 10;
			opt->answers =
			    (char **)G_realloc((char *)opt->answers,
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

    return 0;
}

static int check_multiple_opts(void)
{
    struct Option *opt;
    const char *ptr;
    int n_commas;
    int n;
    int error;

    if (!n_opts)
	return (0);

    error = 0;
    opt = &first_option;
    while (opt != NULL) {
	if ((opt->answer != NULL) && (opt->key_desc != NULL)) {
	    /* count commas */
	    n_commas = 1;
	    for (ptr = opt->key_desc; *ptr != '\0'; ptr++)
		if (*ptr == ',')
		    n_commas++;
	    /* count items */
	    for (n = 0; opt->answers[n] != '\0'; n++) ;
	    /* if not correct multiple of items */
	    if (n % n_commas) {
		fprintf(stderr,
			_
			("\nERROR: option <%s> must be provided in multiples of %d\n"),
			opt->key, n_commas);
		fprintf(stderr, _("       You provided %d items:\n"), n);
		fprintf(stderr, "       %s\n", opt->answer);
		error++;
	    }
	}
	opt = opt->next_opt;
    }
    return (error);
}

/* Check for all 'new' if element already exists */
static int check_overwrite(void)
{
    struct Option *opt;
    char age[KEYLENGTH];
    char element[KEYLENGTH];
    char desc[KEYLENGTH];
    int error = 0;
    char *overstr;
    int over;

    module_info.overwrite = 0;

    if (!n_opts)
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

    if (overwrite || over) {
	module_info.overwrite = 1;
	/* Set the environment so that programs run in a script also obey --o */
	putenv("GRASS_OVERWRITE=1");
	/* No need to check options for existing files if overwrite is true */
	return error;
    }

    opt = &first_option;
    while (opt != NULL) {
	if ((opt->answer != NULL) && (opt->gisprompt != NULL)) {
	    split_gisprompt(opt->gisprompt, age, element, desc);

	    if (strcmp(age, "new") == 0) {
		if (G_find_file(element, opt->answer, G_mapset())) {	/* found */
		    if (!overwrite && !over) {
			if (G_info_format() != G_INFO_FORMAT_GUI) {
			    fprintf(stderr,
				    _("ERROR: option <%s>: <%s> exists.\n"),
				    opt->key, opt->answer);
			}
			else {
			    fprintf(stderr,
				    "GRASS_INFO_ERROR(%d,1): option <%s>: <%s> exists.\n",
				    getpid(), opt->key, opt->answer);
			    fprintf(stderr, "GRASS_INFO_END(%d,1)\n",
				    getpid());
			}

			error = 1;
		    }
		}
	    }
	}
	opt = opt->next_opt;
    }

    return (error);
}

static int split_gisprompt(const char *gisprompt, char *age, char *element,
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

    return 0;
}

/**
 * \brief Creates command to run non-interactive.
 *
 * Creates a command-line that runs the current command completely
 * non-interactive.
 *
 * \return char * Pointer to a char string
 */
char *G_recreate_command(void)
{
    static char *buff;
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

    if (n_flags) {
	flag = &first_flag;
	while (flag != '\0') {
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

    opt = &first_option;
    while (opt != '\0') {
	if (opt->answer != '\0' && opt->answers[0] != NULL) {
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
	    for (n = 1; opt->answers[n] != NULL && opt->answers[n] != '\0';
		 n++) {
		if (opt->answers[n] == NULL)
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

    return (buff);
}
