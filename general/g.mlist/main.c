
/****************************************************************************
 *
 * MODULE:       g.mlist
 *
 * AUTHOR(S):    Huidae Cho
 * 		 Based on general/manage/cmd/list.c by Michael Shapiro.
 *
 * PURPOSE:      Lists available GRASS data base files of the
 *               user-specified data type to standard output
 *
 * COPYRIGHT:    (C) 1999-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/manage.h>
#include <grass/glocale.h>
#include <grass/spawn.h>

static int any = 0;

static void make_list(FILE *, const struct list *, const char *, const char *,
		      int, int, int);

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *type;
	struct Option *pattern;
	struct Option *exclude;
	struct Option *separator;
	struct Option *mapset;
	struct Option *output;
    } opt;
    struct
    {
	struct Flag *regex;
	struct Flag *extended;
	struct Flag *type;
	struct Flag *mapset;
	struct Flag *pretty;
	struct Flag *full;
    } flag;
    int i, n, all, num_types, nlist;
    void *filter = NULL, *exclude = NULL;
    FILE *fp;
    const char *mapset;
    char *separator;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("list"));
    module->description =
	_("Lists available GRASS data base files "
	  "of the user-specified data type optionally using the search pattern.");

    M_read_list(FALSE, &nlist);

    opt.type = G_define_standard_option(G_OPT_M_DATATYPE);
    opt.type->multiple = YES;
    opt.type->options = M_get_options(TRUE);
    opt.type->descriptions = M_get_option_desc(TRUE);
    
    opt.pattern = G_define_option();
    opt.pattern->key = "pattern";
    opt.pattern->type = TYPE_STRING;
    opt.pattern->required = NO;
    opt.pattern->multiple = NO;
    opt.pattern->description = _("Map name search pattern (default: all)");
    opt.pattern->guisection = _("Pattern");

    opt.exclude = G_define_option();
    opt.exclude->key = "exclude";
    opt.exclude->type = TYPE_STRING;
    opt.exclude->required = NO;
    opt.exclude->multiple = NO;
    opt.exclude->description = _("Map name exclusion pattern (default: none)");
    opt.exclude->guisection = _("Pattern");

    opt.mapset = G_define_standard_option(G_OPT_M_MAPSET);
    opt.mapset->label =
	_("Name of mapset to list (default: current search path)");
    opt.mapset->description = _("'.' for current mapset; '..' for all mapsets in location");
    opt.separator = G_define_standard_option(G_OPT_F_SEP);
    opt.separator->answer = "newline";

    opt.output = G_define_standard_option(G_OPT_F_OUTPUT);
    opt.output->required = NO;
    opt.output->label = _("Name for output file");
    opt.output->description = _("If not given or '-' then standard output");

    flag.regex = G_define_flag();
    flag.regex->key = 'r';
    flag.regex->description =
	_("Use basic regular expressions instead of wildcards");
    flag.regex->guisection = _("Pattern");

    flag.extended = G_define_flag();
    flag.extended->key = 'e';
    flag.extended->description =
	_("Use extended regular expressions instead of wildcards");
    flag.extended->guisection = _("Pattern");

    flag.type = G_define_flag();
    flag.type->key = 't';
    flag.type->description = _("Print data types");
    flag.type->guisection = _("Print");
    
    flag.mapset = G_define_flag();
    flag.mapset->key = 'm';
    flag.mapset->description = _("Print fully-qualified map names (including mapsets)");
    flag.mapset->guisection = _("Print");

    flag.pretty = G_define_flag();
    flag.pretty->key = 'p';
    flag.pretty->description = _("Pretty printing in human readable format");
    flag.pretty->guisection = _("Print");

    flag.full = G_define_flag();
    flag.full->key = 'f';
    flag.full->description = _("Verbose listing (also list map titles)");
    flag.full->guisection = _("Print");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if ((flag.pretty->answer || flag.full->answer) && opt.output->answer)
        G_fatal_error(_("-%c/-%c and %s= are mutually exclusive"),
		      flag.pretty->key, flag.full->key, opt.output->key);

    if ((flag.pretty->answer || flag.full->answer) && flag.type->answer)
	G_fatal_error(_("-%c/-%c and -%c are mutually exclusive"),
		      flag.pretty->answer, flag.full->key, flag.type->answer);

    if (flag.pretty->answer && flag.full->answer)
	G_fatal_error(_("-%c and -%c are mutually exclusive"),
		      flag.pretty->answer, flag.full->answer);

    if (flag.regex->answer && flag.extended->answer)
	G_fatal_error(_("-%c and -%c are mutually exclusive"),
		      flag.regex->answer, flag.extended->answer);

    if (opt.pattern->answer) {
	if (flag.regex->answer || flag.extended->answer)
	    filter = G_ls_regex_filter(opt.pattern->answer, 0,
			    	       (int)flag.extended->answer);
	else
	    filter = G_ls_glob_filter(opt.pattern->answer, 0);
    }

    if (opt.exclude->answer) {
	if (flag.regex->answer || flag.extended->answer)
	    exclude = G_ls_regex_filter(opt.exclude->answer, 1,
			    		(int)flag.extended->answer);
	else
	    exclude = G_ls_glob_filter(opt.exclude->answer, 1);
    }

    separator = G_option_to_separator(opt.separator);
    fp = G_open_option_file(opt.output);

    if ((mapset = opt.mapset->answer) == NULL)
	mapset = "";
    else if (strcmp(mapset, ".") == 0)
	mapset = G_mapset();   /* current mapset */
    else if (strcmp(mapset, "..") == 0) {
        if (flag.pretty->answer || flag.full->answer)
            G_fatal_error(_("-%c/-%c and %s=%s are mutually exclusive"),
                          flag.pretty->key, flag.full->key, opt.mapset->key,
			  mapset);
        mapset = NULL;         /* all mapsets */
    }

    for (i = 0; opt.type->answers[i]; i++) {
	if (strcmp(opt.type->answers[i], "all") == 0)
	    break;
    }
    if (opt.type->answers[i]) {
	all = 1;
	num_types = nlist;
    }
    else {
	all = 0;
	num_types = i;
    }

    for (i = 0; i < num_types; i++) {
	const struct list *elem;

	n = all ? i : M_get_element(opt.type->answers[i]);

	elem = M_get_list(n);

	if (flag.full->answer) {
	    char lister[GPATH_MAX];

	    sprintf(lister, "%s/etc/lister/%s", G_gisbase(), elem->element[0]);

	    G_debug(3, "lister CMD: %s", lister);

	    if (access(lister, 1) == 0) {	/* execute permission? */
		G_spawn(lister, lister, mapset, NULL);
		continue;
	    }
	}
	else if (flag.pretty->answer)
	    G_list_element(elem->element[0], elem->alias, mapset, NULL);
	else
	    make_list(fp, elem, mapset, separator, flag.type->answer,
		      flag.mapset->answer, mapset && *mapset);
    }

    if (any)
	fprintf(fp, "\n");

    G_close_option_file(fp);

    if (filter)
	G_free_ls_filter(filter);

    if (exclude)
	G_free_ls_filter(exclude);

    exit(EXIT_SUCCESS);
}

static void make_list(FILE *fp, const struct list *elem, const char *mapset,
		      const char *separator, int add_type, int add_mapset,
		      int single_mapset)
{
    char path[GPATH_MAX];
    const char *element = elem->element[0];
    const char *alias = elem->alias;
    char **list;
    int count;
    int i;

    if (!mapset) {
        /* all mapsets from current location */
        int n;
        char **ms;
        ms = G_get_available_mapsets();
        for (n = 0; ms[n]; n++) {
            make_list(fp, elem, ms[n], separator, add_type, add_mapset,
		      n == 0);
        }
        return;
    }
    else if (!*mapset) {
        /* mapsets from search path only */
	int n;
	for (n = 0; mapset = G_get_mapset_name(n), mapset; n++)
	    make_list(fp, elem, mapset, separator, add_type, add_mapset,
		      n == 0);
	return;
    }

    G_file_name(path, element, "", mapset);
    if (access(path, 0) != 0)
	return;

    if ((list = G__ls(path, &count)) == NULL)
	return;

    if (count > 0) {
	if (any)
	    fprintf(fp, "%s", separator);
	if (fp == stdout)
	    G_message(_("%s available in mapset <%s>:"), elem->text, mapset);
    }

    /* Suppress "... found in more mapsets" warnings from G_find_file2. */
    G_suppress_warnings(1);

    for (i = 0; i < count; i++) {
	char *name = list[i];
	int need_mapset = 0;

	if (any && i != 0)
	    fprintf(fp, "%s", separator);
	
	if (add_type)
	    fprintf(fp, "%s/", alias);

	fprintf(fp, "%s", name);

	if (!add_mapset && !single_mapset) {
	    const char *mapset2 = G_find_file2(element, name, "");
            if (mapset2)
                need_mapset = strcmp(mapset, mapset2) != 0;
	}
	if (add_mapset || need_mapset)
	    fprintf(fp, "@%s", mapset);

	G_free(name);

	any++;
    }

    G_suppress_warnings(0);
    fflush(fp);
    
    G_free(list);
}

