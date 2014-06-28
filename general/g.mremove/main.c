
/****************************************************************************
 *
 * MODULE:       g.mremove
 *
 * AUTHOR(S):    Huidae Cho <grass4u gmail.com>
 *
 * 		 Based on general/manage/cmd/remove.c by
 *               CERL (original contributor),
 *               Radim Blazek <radim.blazek gmail.com>,
 *               Cedric Shock <cedricgrass shockfamily.net>,
 *               Huidae Cho <grass4u gmail.com>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>,
 *               Markus Neteler <neteler itc.it>,
 *               Martin Landa <landa.martin gmail.com>
 *
 * PURPOSE:      lets users remove GRASS database files
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
    } opt;
    struct
    {
	struct Flag *regex;
	struct Flag *extended;
	struct Flag *force;
	struct Flag *basemap;
    } flag;
    const char *mapset;
    int result;
    int i, all, num_types, nlist;
    void *filter, *exclude;

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

    opt.pattern = G_define_option();
    opt.pattern->key = "pattern";
    opt.pattern->type = TYPE_STRING;
    opt.pattern->required = YES;
    opt.pattern->description = _("Map name search pattern");
    opt.pattern->guisection = _("Pattern");

    opt.exclude = G_define_option();
    opt.exclude->key = "exclude";
    opt.exclude->type = TYPE_STRING;
    opt.exclude->required = NO;
    opt.exclude->description = _("Map name exclusion pattern (default: none)");
    opt.exclude->guisection = _("Pattern");

    flag.regex = G_define_flag();
    flag.regex->key = 'r';
    flag.regex->description =
	_("Use basic regular expressions instead of wildcards");

    flag.extended = G_define_flag();
    flag.extended->key = 'e';
    flag.extended->description =
	_("Use extended regular expressions instead of wildcards");

    flag.force = G_define_flag();
    flag.force->key = 'f';
    flag.force->description =
	_("Force removal (required for actual deletion of files)");

    flag.basemap = G_define_flag();
    flag.basemap->key = 'b';
    flag.basemap->description = _("Remove base raster maps");
    flag.basemap->guisection = _("Raster");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (flag.regex->answer && flag.extended->answer)
	G_fatal_error(_("-%c and -%c are mutually exclusive"),
		      flag.regex->key, flag.extended->key);

    if (flag.regex->answer || flag.extended->answer)
	filter = G_ls_regex_filter(opt.pattern->answer, 0,
				   (int)flag.extended->answer);
    else {
	/* handle individual map names */
	if (strchr(opt.pattern->answer, ',')) {
	    char *pattern;

	    pattern = (char *)G_malloc(strlen(opt.pattern->answer) + 3);
	    sprintf(pattern, "{%s}", opt.pattern->answer);

	    filter = G_ls_glob_filter(pattern, 0);
	}
	else
	    filter = G_ls_glob_filter(opt.pattern->answer, 0);
    }
    if (!filter)
	G_fatal_error(_("Unable to compile pattern <%s>"), opt.pattern->answer);

    if (opt.exclude->answer) {
	if (flag.regex->answer || flag.extended->answer)
	    exclude = G_ls_regex_filter(opt.exclude->answer, 1,
			    		(int)flag.extended->answer);
	else {
	    /* handle individual map names */
	    if (strchr(opt.exclude->answer, ',')) {
		char *pattern;

		pattern = (char *)G_malloc(strlen(opt.exclude->answer) + 3);
		sprintf(pattern, "{%s}", opt.exclude->answer);

		exclude = G_ls_glob_filter(pattern, 1);
	    }
	    else
		exclude = G_ls_glob_filter(opt.exclude->answer, 1);
	}
	if (!exclude)
	    G_fatal_error(_("Unable to compile pattern <%s>"),
			  opt.exclude->answer);
    }
    else
	exclude = NULL;

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

	rast = !G_strcasecmp(elem->alias, "rast");
	files = G__ls(path, &num_files);
	
	for (j = 0; j < num_files; j++) {
	    if (!flag.force->answer) {
		fprintf(stdout, "%s/%s@%s\n", elem->alias, files[j], mapset);
		continue;
	    }
	    
	    if (rast && check_reclass(files[j], mapset, flag.basemap->answer))
		continue;
	    
	    if (M_do_remove(n, (char *)files[j]) == 1)
		result = EXIT_FAILURE;
	}
    }

    G_free_ls_filter(filter);

    if (exclude)
	G_free_ls_filter(exclude);

    if (!flag.force->answer)
	G_important_message(_("You must use the force flag (-%c) to actually "
			      "remove them. Exiting."), flag.force->key);

    exit(result);
}
