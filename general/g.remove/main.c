
/****************************************************************************
 *
 * MODULE:       g.remove
 *
 * AUTHOR(S):    Huidae Cho <grass4u gmail.com>
 *
 * 		 Based on general/manage/cmd/remove.c by
 *               CERL (original contributor),
 *               Radim Blazek <radim.blazek gmail.com>,
 *               Cedric Shock <cedricgrass shockfamily.net>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>,
 *               Markus Neteler <neteler itc.it>,
 *               Martin Landa <landa.martin gmail.com>
 *
 * PURPOSE:      Lets users remove GRASS database files
 *
 * COPYRIGHT:    (C) 1999-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/manage.h>
#include <grass/glocale.h>

/* construct_pattern.c */
char *construct_pattern(char **);
/* check_reclass.c */
int check_reclass(const char *, const char *, int);

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *type;
	struct Option *pattern;
	struct Option *exclude;
	struct Option *name;
	struct Option *ignore;
    } opt;
    struct
    {
	struct Flag *ignorecase;
	struct Flag *regex;
	struct Flag *extended;
	struct Flag *force;
	struct Flag *basemap;
    } flag;
    char *pattern, *exclude;
    const char *mapset;
    int result;
    int i, all, num_types, nlist, num_removed;
    void *filter, *exclude_filter;

    G_gisinit(argv[0]);

    result = EXIT_SUCCESS;

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("remove"));
    module->description =
	_("Removes data base element files from "
	  "the user's current mapset using the search pattern.");

    M_read_list(FALSE, &nlist);

    opt.type = G_define_standard_option(G_OPT_M_DATATYPE);
    opt.type->multiple = YES;
    opt.type->options = M_get_options(TRUE);
    opt.type->descriptions = M_get_option_desc(TRUE);
    opt.type->guidependency = "pattern,exclude,name,ignore";
    opt.type->guisection = _("Basic");

    opt.name = G_define_option();
    opt.name->key = "name";
    opt.name->type = TYPE_STRING;
    opt.name->multiple = YES;
    opt.name->gisprompt = "old,element,element";
    opt.name->description = _("Name of file(s) to remove");
    opt.name->guisection = _("Basic");

    opt.ignore = G_define_option();
    opt.ignore->key = "ignore";
    opt.ignore->type = TYPE_STRING;
    opt.ignore->multiple = YES;
    opt.ignore->gisprompt = "old,element,element";
    opt.ignore->description =
	_("Name of file(s) to ignore (default: none)");
    opt.ignore->guisection = _("Pattern");

    opt.pattern = G_define_option();
    opt.pattern->key = "pattern";
    opt.pattern->type = TYPE_STRING;
    opt.pattern->description = _("File name search pattern");
    opt.pattern->guisection = _("Pattern");

    opt.exclude = G_define_option();
    opt.exclude->key = "exclude";
    opt.exclude->type = TYPE_STRING;
    opt.exclude->description = _("File name exclusion pattern (default: none)");
    opt.exclude->guisection = _("Pattern");

    flag.ignorecase = G_define_flag();
    flag.ignorecase->key = 'i';
    flag.ignorecase->description = _("Ignore case");
    flag.ignorecase->guisection = _("Pattern");

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

    flag.force = G_define_flag();
    flag.force->key = 'f';
    flag.force->guisection = _("Basic");
    flag.force->description =
	_("Force removal (required for actual deletion of files)");

    flag.basemap = G_define_flag();
    flag.basemap->key = 'b';
    flag.basemap->description = _("Remove base raster maps");

    G_option_exclusive(flag.regex, flag.extended, NULL);
    G_option_exclusive(opt.pattern, opt.name, NULL);
    G_option_exclusive(opt.exclude, opt.ignore, NULL);
    G_option_required(opt.pattern, opt.name, NULL);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (opt.pattern->answer)
	pattern = opt.pattern->answer;
    else
	pattern = construct_pattern(opt.name->answers);

    if (opt.exclude->answer)
	exclude = opt.exclude->answer;
    else if (opt.ignore->answer)
	exclude = construct_pattern(opt.ignore->answers);
    else
	exclude = NULL;

    if ((flag.regex->answer || flag.extended->answer) && opt.pattern->answer)
	filter = G_ls_regex_filter(pattern, 0, (int)flag.extended->answer,
				   (int)flag.ignorecase->answer);
    else {
	/* handle individual map names */
	if (strchr(pattern, ',')) {
	    char *buf;

	    buf = (char *)G_malloc(strlen(pattern) + 3);
	    sprintf(buf, "{%s}", pattern);

	    filter = G_ls_glob_filter(buf, 0, (int)flag.ignorecase->answer);
	}
	else
	    filter = G_ls_glob_filter(pattern, 0, (int)flag.ignorecase->answer);
    }
    if (!filter)
	G_fatal_error(_("Unable to compile pattern <%s>"), pattern);

    if (exclude) {
	if ((flag.regex->answer || flag.extended->answer) &&
	    opt.exclude->answer)
	    exclude_filter = G_ls_regex_filter(exclude, 1,
					       (int)flag.extended->answer,
					       (int)flag.ignorecase->answer);
	else {
	    /* handle individual map names */
	    if (strchr(exclude, ',')) {
		char *buf;

		buf = (char *)G_malloc(strlen(exclude) + 3);
		sprintf(buf, "{%s}", exclude);

		exclude_filter = G_ls_glob_filter(buf, 1,
						  (int)flag.ignorecase->answer);
	    }
	    else
		exclude_filter = G_ls_glob_filter(exclude, 1,
						  (int)flag.ignorecase->answer);
	}
	if (!exclude_filter)
	    G_fatal_error(_("Unable to compile pattern <%s>"), exclude);
    }
    else
	exclude_filter = NULL;

    if (!flag.force->answer)
	G_message(_("The following data base element files would be deleted:"));

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

    num_removed = 0;
    for (i = 0; i < num_types; i++) {
	int n, rast, num_files, j;
	const struct list *elem;
    	char path[GPATH_MAX];
	char **files;

	n = all ? i : M_get_element(opt.type->answers[i]);
	elem = M_get_list(n);

	G_file_name(path, elem->element[0], "", mapset);
	if (access(path, 0) != 0)
	    continue;

	rast = !G_strcasecmp(elem->alias, "raster");
	files = G_ls2(path, &num_files);

	for (j = 0; j < num_files; j++) {
	    if (!flag.force->answer) {
		fprintf(stdout, "%s/%s@%s\n", elem->alias, files[j], mapset);
                num_removed++;
		continue;
	    }

	    if (rast && check_reclass(files[j], mapset, flag.basemap->answer))
		continue;

	    if (M_do_remove(n, (char *)files[j]) == 1)
		result = EXIT_FAILURE;
            num_removed++;
	}
    }

    if (num_removed < 1)
        G_warning(_("No data base element files found"));
    
    G_free_ls_filter(filter);

    if (exclude_filter)
	G_free_ls_filter(exclude_filter);

    if (!flag.force->answer && num_removed > 0)
	G_warning(_("Nothing removed. You must use the force flag (-%c) to actually "
                    "remove them. Exiting."), flag.force->key);

    exit(result);
}
