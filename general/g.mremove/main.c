
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
 * COPYRIGHT:    (C) 1999-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
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
    const char *mapset, *location_path;
    char *name, path[GPATH_MAX], **files;
    char *alias;
    int num_files, rast, result = EXIT_SUCCESS;
    int i, j, n, nlist;
    void *filter;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("remove"));
    G_add_keyword(_("multi"));
    module->description =
	_("Removes data base element files from "
	  "the user's current mapset.");

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
    flag.basemap->description = _("Remove base maps");

    M_read_list(FALSE, &nlist);

    opt = (struct Option **)G_calloc(nlist, sizeof(struct Option *));

    for (n = 0; n < nlist; n++) {
	o = opt[n] = M_define_option(n, _("removed"), YES);
    }

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (flag.regex->answer && flag.extended->answer)
	G_fatal_error(_("-r and -e are mutually exclusive"));

    if (!flag.force->answer)
	G_message(_("The following files would be deleted:"));

    for (n = 0; n < nlist; n++) {
	o = opt[n];
	G_free((char *)o->gisprompt);
	G_free((char *)o->description);
    }

    location_path = G_location_path();
    mapset = G_mapset();

    for (n = 0; n < nlist; n++) {
	alias = M_get_list(n)->alias;
	if (opt[n]->answers) {
	    G_file_name(path, M_get_list(n)->element[0], "", mapset);
	    if (access(path, 0) != 0)
		continue;
	    rast = !G_strcasecmp(alias, "rast");
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
			fprintf(stdout, "%s/%s@%s\n", alias, files[j],
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
	G_message(" ");
	G_message(_("You must use the force flag to actually remove them. Exiting."));
    }

    exit(result);
}

