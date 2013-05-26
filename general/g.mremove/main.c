
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
 * COPYRIGHT:    (C) 1999-2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/manage.h>
#include <grass/glocale.h>

/* check_reclass.c */
int check_reclass(const char *, const char *, int);

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option **opt, *o;
    struct
    {
	struct Flag *regex;
	struct Flag *extended;
	struct Flag *force;
	struct Flag *basemap;
    } flag;
    const char *mapset;
    char *name, path[GPATH_MAX], **files;
    const struct list *option;
    int num_files, rast, result;
    int i, j, n, nlist;
    void *filter;

    G_gisinit(argv[0]);

    result = EXIT_SUCCESS;
    
    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("remove"));
    module->description =
	_("Removes data base element files from "
	  "the user's current mapset using regular expressions.");
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
    
    M_read_list(FALSE, &nlist);

    opt = (struct Option **)G_calloc(nlist, sizeof(struct Option *));

    for (n = 0; n < nlist; n++) {
	o = opt[n] = M_define_option(n, _("removed"), YES);
    }

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (flag.regex->answer && flag.extended->answer)
	G_fatal_error(_("-%c and -%c are mutually exclusive"),
		      flag.regex->key, flag.extended->key);

    if (!flag.force->answer)
	G_message(_("The following data base element files would be deleted:"));

    for (n = 0; n < nlist; n++) {
	o = opt[n];
	G_free((char *)o->gisprompt);
	G_free((char *)o->description);
    }

    mapset = G_mapset();

    for (n = 0; n < nlist; n++) {
	option = M_get_list(n);
	if (opt[n]->answers) {
	    G_file_name(path, M_get_list(n)->element[0], "", mapset);
	    if (access(path, 0) != 0)
		continue;
	    rast = !G_strcasecmp(option->alias, "rast");
	    for (i = 0; (name = opt[n]->answers[i]); i++) {
		if (!flag.regex->answer && !flag.extended->answer)
		    filter = G_ls_glob_filter(name, 0);
		else
		    filter = G_ls_regex_filter(name, 0,
					       (int) flag.extended->answer);
		if (!filter)
		    G_fatal_error(_("Unable to compile pattern <%s>"),
				  name);

		files = G__ls(path, &num_files);

		G_free_ls_filter(filter);

		for (j = 0; j < num_files; j++) {
		    if (!flag.force->answer) {
			fprintf(stdout, "%s/%s@%s\n", option->alias, files[j],
				mapset);
			continue;
		    }
		    if (rast &&
			check_reclass(files[j], mapset, flag.basemap->answer))
			continue;

		    if (M_do_remove(n, (char *)files[j]) == 1)
			result = EXIT_FAILURE;
		}
	    }
	}
    }

    if (!flag.force->answer) {
	G_important_message(_("You must use the force flag (-%c) to actually "
			      "remove them. Exiting."), flag.force->key);
    }

    exit(result);
}

