
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
 * COPYRIGHT:    (C) 1999-2010 by the GRASS Development Team
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
#include <grass/list.h>
#include <grass/glocale.h>
#include <grass/spawn.h>

static int any = 0;

static void make_list(const struct list *,
		      const char *, const char *,
		      int, int, int);
static int parse(const char *);

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
    int i, n, all, num_types;
    void *filter = NULL, *exclude = NULL;
    const char *mapset;
    char separator[2], *buf;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    module->description =
	_("Lists available GRASS data base files "
	  "of the user-specified data type to standard output.");

    read_list(0);

    opt.type = G_define_option();
    opt.type->key = "type";
    opt.type->key_desc = "datatype";
    opt.type->type = TYPE_STRING;
    opt.type->required = YES;
    opt.type->multiple = YES;
    opt.type->answer = "rast";
    opt.type->description = "Data type";
    for (i = 0, n = 0; n < nlist; n++)
	i += strlen(list[n].alias) + 1;
    buf = G_malloc(i + 4);

    buf[0] = 0;
    for (n = 0; n < nlist; n++) {
	strcat(buf, list[n].alias);
	strcat(buf, ",");
    }
    strcat(buf, "all");
    opt.type->options = buf;

    opt.pattern = G_define_option();
    opt.pattern->key = "pattern";
    opt.pattern->type = TYPE_STRING;
    opt.pattern->required = NO;
    opt.pattern->multiple = NO;
    opt.pattern->description = _("Map name search pattern (default: all)");

    opt.exclude = G_define_option();
    opt.exclude->key = "exclude";
    opt.exclude->type = TYPE_STRING;
    opt.exclude->required = NO;
    opt.exclude->multiple = NO;
    opt.exclude->description = _("Map name exclusion pattern (default: none)");

    opt.separator = G_define_option();
    opt.separator->key = "separator";
    opt.separator->type = TYPE_STRING;
    opt.separator->required = NO;
    opt.separator->multiple = NO;
    opt.separator->answer = "newline";
    opt.separator->description =
	_("One-character output separator, newline, comma, space, or tab");

    opt.mapset = G_define_option();
    opt.mapset->key = "mapset";
    opt.mapset->type = TYPE_STRING;
    opt.mapset->required = NO;
    opt.mapset->multiple = NO;
    opt.mapset->description =
	_("Mapset to list (default: current search path)");

    flag.regex = G_define_flag();
    flag.regex->key = 'r';
    flag.regex->description =
	_("Use basic regular expressions instead of wildcards");

    flag.extended = G_define_flag();
    flag.extended->key = 'e';
    flag.extended->description =
	_("Use extended regular expressions instead of wildcards");

    flag.type = G_define_flag();
    flag.type->key = 't';
    flag.type->description = _("Print data types");

    flag.mapset = G_define_flag();
    flag.mapset->key = 'm';
    flag.mapset->description = _("Print mapset names");

    flag.pretty = G_define_flag();
    flag.pretty->key = 'p';
    flag.pretty->description = _("Pretty printing in human readable format");

    flag.full = G_define_flag();
    flag.full->key = 'f';
    flag.full->description = _("Verbose listing (also list map titles)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_free(buf);

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
	    exclude = G_ls_regex_filter(opt.pattern->answer, 1, (int) flag.extended->answer);
	else
	    exclude = G_ls_glob_filter(opt.pattern->answer, 1);
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
	n = all ? i : parse(opt.type->answers[i]);

	if (flag.full->answer) {
	    char lister[GPATH_MAX];

	    sprintf(lister, "%s/etc/lister/%s", G_gisbase(),
		    list[n].element[0]);

	    G_debug(3, "lister CMD: %s", lister);

	    if (access(lister, 1) == 0) {	/* execute permission? */
		G_spawn(lister, lister, mapset, NULL);
		continue;
	    }
	}
	else
	    make_list(&list[n], mapset, separator,
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

    for (i = 0; i < count; i++) {
	char *name = list[i];

	if (any)
	    fprintf(stdout, "%s", separator);

	if (add_type)
	    fprintf(stdout, "%s/", alias);

	fprintf(stdout, "%s", name);

	if (add_mapset)
	    fprintf(stdout, "@%s", mapset);

	G_free(name);

	any++;
    }

    G_free(list);
}

static int parse(const char *data_type)
{
    int n;

    for (n = 0; n < nlist; n++) {
	if (G_strcasecmp(list[n].alias, data_type) == 0)
	    break;
    }

    return n;
}

