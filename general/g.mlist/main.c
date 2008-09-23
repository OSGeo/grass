
/****************************************************************************
 *
 * MODULE:       g.xlist
 *
 * AUTHOR(S):    Huidae Cho
 * 		 Based on general/manage/cmd/list.c by Michael Shapiro.
 *
 * PURPOSE:      Lists available GRASS data base files of the
 *               user-specified data type to standard output
 *
 * COPYRIGHT:    (C) 1999-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <grass/spawn.h>
#include "global.h"

int nlist;
struct list *list;

static int any;

static void do_list(const struct list *,
		    const char *, const char *, const char *,
		    int, int, int);
static int parse(const char *);
static int ls_filter(const char *, void *);

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *type;
	struct Option *pattern;
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
    char *pattern = NULL, separator[2], *buf;
    regex_t regex;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("general, map management");
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
    opt.pattern->answer = "*";
    opt.pattern->description = _("Map name search pattern (default: all)");

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

    if (flag.regex->answer || flag.extended->answer)
	pattern = opt.pattern->answer;
    else
	pattern = wc2regex(opt.pattern->answer);

    if (regcomp(&regex, pattern, (flag.regex->answer ? 0 : REG_EXTENDED) | REG_NOSUB))
	G_fatal_error(_("Unable to compile regular expression %s"), pattern);
    G_set_ls_filter(ls_filter, &regex);

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

    if (opt.mapset->answer == NULL)
	opt.mapset->answer = "";

    if (G_strcasecmp(opt.mapset->answer, ".") == 0)
	opt.mapset->answer = G_mapset();

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
		G_spawn(lister, lister, opt.mapset->answer, NULL);
		continue;
	    }
	}
	else
	    do_list(&list[n], pattern, opt.mapset->answer, separator,
		    flag.pretty->answer, flag.type->answer,
		    flag.mapset->answer);
    }

    if (!flag.pretty->answer && any)
	fprintf(stdout, "\n");

    if (!flag.regex->answer)
	G_free(pattern);

    regfree(&regex);

    exit(EXIT_SUCCESS);
}

static void do_list(
    const struct list *elem,
    const char *pattern, const char *mapset, const char *separator,
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
	    do_list(elem, pattern, mapset, separator,
		    pretty, add_type, add_mapset);
	return;
    }

    G__file_name(path, element, "", mapset);
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

static int ls_filter(const char *filename, void *closure)
{
    return filename[0] != '.' &&
	regexec((regex_t *) closure, filename, 0, NULL, 0) == 0;
}
