
/****************************************************************************
 *
 * MODULE:       g.xremove
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
 * COPYRIGHT:    (C) 1999-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include "global.h"

int nlist;
struct list *list;

static int ls_filter(const char *, void *);

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
    char *name, *mapset, *location_path, path[GPATH_MAX], **files;
    char *buf, *buf2;
    int num_files, rast, result = EXIT_SUCCESS;
    int i, j, n;
    regex_t regex;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("general, map management");
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

    read_list(0);

    opt = (struct Option **)G_calloc(nlist, sizeof(struct Option *));

    for (n = 0; n < nlist; n++) {
	o = opt[n] = G_define_option();
	o->key = list[n].alias;
	o->type = TYPE_STRING;
	o->required = NO;
	o->multiple = YES;
	buf = G_malloc(64);
	sprintf(buf, "old,%s,%s", list[n].mainelem, list[n].maindesc);
	o->gisprompt = buf;
	buf2 = G_malloc(64);
	sprintf(buf2, _("%s file(s) to be removed"), list[n].alias);
	o->description = buf2;
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
	if (opt[n]->answers) {
	    G__file_name(path, list[n].element[0], "", mapset);
	    if (access(path, 0) != 0)
		continue;
	    rast = !G_strcasecmp(list[n].alias, "rast");
	    for (i = 0; (name = opt[n]->answers[i]); i++) {
		if (!flag.regex->answer && !flag.extended->answer)
		    name = wc2regex(name);
		if (regcomp(&regex, name,
			    (flag.regex->answer ? 0 : REG_EXTENDED) | REG_NOSUB))
		    G_fatal_error(
				  _("Unable to compile regular expression %s"),
				  name);
		if (!flag.regex->answer && !flag.extended->answer)
		    G_free(name);

		G_set_ls_filter(ls_filter, &regex);
		files = G__ls(path, &num_files);
		regfree(&regex);

		for (j = 0; j < num_files; j++) {
		    if (!flag.force->answer) {
			fprintf(stdout, "%s/%s@%s\n", list[n].alias, files[j],
				mapset);
			continue;
		    }
		    if (rast &&
			check_reclass(files[j], mapset, flag.basemap->answer))
			continue;

		    if (do_remove(n, (char *)files[j]) == 1)
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

static int ls_filter(const char *filename, void *closure)
{
    return filename[0] != '.' &&
	regexec((regex_t *) closure, filename, 0, NULL, 0) == 0;
}
