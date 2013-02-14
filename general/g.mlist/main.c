
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
 * COPYRIGHT:    (C) 1999-2011 by the GRASS Development Team
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

static void make_list(const struct list *,
		      const char *, const char *,
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
    const char *mapset;
    char separator[2];

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

    opt.separator = G_define_standard_option(G_OPT_F_SEP);
    opt.separator->answer = "newline";

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

    if (flag.regex->answer && flag.extended->answer)
	G_fatal_error(_("-r and -e are mutually exclusive"));

    if (opt.pattern->answer) {
	if (flag.regex->answer || flag.extended->answer)
	    filter = G_ls_regex_filter(opt.pattern->answer, 0, (int) flag.extended->answer);
	else
	    filter = G_ls_glob_filter(opt.pattern->answer, 0);
    }

    if (opt.exclude->answer) {
	if (flag.regex->answer || flag.extended->answer)
	    exclude = G_ls_regex_filter(opt.exclude->answer, 1, (int) flag.extended->answer);
	else
	    exclude = G_ls_glob_filter(opt.exclude->answer, 1);
    }

    if (strcmp(opt.separator->answer, "newline") == 0)
	separator[0] = '\n';
    else if (strcmp(opt.separator->answer, "comma") == 0)
	separator[0] = ',';
    else if (strcmp(opt.separator->answer, "space") == 0)
	separator[0] = ' ';
    else if (strcmp(opt.separator->answer, "tab") == 0)
	separator[0] = '\t';
    else
	separator[0] = opt.separator->answer[0];
    separator[1] = 0;

    mapset = opt.mapset->answer;

    if (mapset == NULL)
	mapset = "";

    if (G_strcasecmp(mapset, ".") == 0)
	mapset = G_mapset();

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
	n = all ? i : M_get_element(opt.type->answers[i]);

	if (flag.full->answer) {
	    char lister[GPATH_MAX];

	    sprintf(lister, "%s/etc/lister/%s", G_gisbase(),
		    M_get_list(n)->element[0]);

	    G_debug(3, "lister CMD: %s", lister);

	    if (access(lister, 1) == 0) {	/* execute permission? */
		G_spawn(lister, lister, mapset, NULL);
		continue;
	    }
	}
	else
	    make_list(M_get_list(n), mapset, separator,
		      flag.pretty->answer, flag.type->answer,
		      flag.mapset->answer);
    }

    if (!flag.pretty->answer && any)
	fprintf(stdout, "\n");

    if (filter)
	G_free_ls_filter(filter);

    if (exclude)
	G_free_ls_filter(exclude);

    exit(EXIT_SUCCESS);
}

static void make_list(
    const struct list *elem,
    const char *mapset, const char *separator,
    int pretty, int add_type, int add_mapset)
{
    char path[GPATH_MAX];
    const char *element = elem->element[0];
    const char *alias = elem->alias;
    char **list;
    int count;
    int i;

    if (pretty) {
	G_list_element(element, alias, mapset, NULL);
	return;
    }

    if (!mapset || !*mapset) {
	int n;
	for (n = 0; mapset = G__mapset_name(n), mapset; n++)
	    make_list(elem, mapset, separator,
		      pretty, add_type, add_mapset);
	return;
    }

    G_file_name(path, element, "", mapset);
    if (access(path, 0) != 0)
	return;

    list = G__ls(path, &count);
    if (!list)
	return;

    if (count > 0) {
	if (any) {
	    if (pretty)
		fprintf(stdout, "\n");
	    else
		fprintf(stdout, "%s", separator);
	}
	G_message(_("%s available in mapset <%s>:"),
		  elem->text, mapset);
    }
    
    for (i = 0; i < count; i++) {
	char *name = list[i];
	int need_mapset = 0;

	if (any && i != 0)
	    fprintf(stdout, "%s", separator);
	
	if (add_type)
	    fprintf(stdout, "%s/", alias);

	fprintf(stdout, "%s", name);

	if (!add_mapset) {
	    const char *mapset2 = G_find_file2(element, name, "");
	    need_mapset = strcmp(mapset, mapset2) != 0;
	}
	if (add_mapset || need_mapset)
	    fprintf(stdout, "@%s", mapset);

	G_free(name);

	any++;
    }

    fflush(stdout);
    
    G_free(list);
}

