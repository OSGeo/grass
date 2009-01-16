
/****************************************************************************
 *
 * MODULE:       g.mapsets
 * AUTHOR(S):    Michael Shapiro (CERL), Greg Koerper (ManTech Environmental
 *                    Technology) (original contributors), 
 *               Glynn Clements <glynn gclements.plus.com>
 *               Hamish Bowman <hamish_nospam yahoo.com>, 
 *               Markus Neteler <neteler itc.it>, 
 *               Moritz Lennert <mlennert club.worldonline.be>
 *               Martin Landa <landa.martin gmail.com>
 * PURPOSE:      set current mapset path
 * COPYRIGHT:    (C) 1994-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/spawn.h>
#include <grass/glocale.h>
#include "local_proto.h"
#include "externs.h"

char *mapset_name[GMAPSET_MAX];
int nmapsets;
int choice[GMAPSET_MAX];
int nchoices;
int curr_mapset[GMAPSET_MAX];
int ncurr_mapsets;

static char Path[GPATH_MAX];

int main(int argc, char *argv[])
{
    int n;
    int i;
    int skip;
    const char *cur_mapset;
    char **ptr;
    char **tokens;
    int no_tokens;
    FILE *fp;
    char path[GPATH_MAX];

    struct GModule *module;    
    struct _opt {
	struct Option *mapset, *add, *remove;
	struct Option *fs;
	struct Flag *print;
	struct Flag *list;
	struct Flag *dialog;
    } opt;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("general, settings");
    module->description =
	_("Modifies the user's current mapset "
	  "search path, affecting the user's access to data existing "
	  "under the other GRASS mapsets in the current location.");

    opt.mapset = G_define_option();
    opt.mapset->key = "mapset";
    opt.mapset->type = TYPE_STRING;
    opt.mapset->required = NO;
    opt.mapset->multiple = YES;
    opt.mapset->description = _("Name(s) of existing mapset(s)");

    opt.add = G_define_option();
    opt.add->key = "addmapset";
    opt.add->type = TYPE_STRING;
    opt.add->required = NO;
    opt.add->multiple = YES;
    opt.add->description =
	_("Name(s) of existing mapset(s) to add to search list");

    opt.remove = G_define_option();
    opt.remove->key = "removemapset";
    opt.remove->type = TYPE_STRING;
    opt.remove->required = NO;
    opt.remove->multiple = YES;
    opt.remove->description =
	_("Name(s) of existing mapset(s) to remove from search list");
    
    opt.fs = G_define_standard_option(G_OPT_F_SEP);
    opt.fs->answer = " ";
    
    opt.list = G_define_flag();
    opt.list->key = 'l';
    opt.list->description = _("List all available mapsets");

    opt.print = G_define_flag();
    opt.print->key = 'p';
    opt.print->description = _("Print current mapset search path");

    opt.dialog = G_define_flag();
    opt.dialog->key = 's';
    opt.dialog->description = _("Show mapset selection dialog");

    Path[0] = '\0';
    nchoices = 0;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (opt.list->answer) {
	get_available_mapsets();
	display_available_mapsets(opt.fs->answer);
    }

    if (opt.dialog->answer) {
	sprintf(path, "%s/etc/gui/g.mapsets.py", G_gisbase());
	G_spawn("python", "g.mapsets.py", path, NULL);
    }

    if (opt.mapset->answer) {
	for (ptr = opt.mapset->answers; *ptr != NULL; ptr++) {
	    const char *mapset;

	    mapset = *ptr;
	    if (G__mapset_permissions(mapset) < 0)
		G_fatal_error(_("Mapset <%s> not found"), mapset);
	    nchoices++;
	    strcat(Path, mapset);
	    strcat(Path, " ");
	}
    }

    /* add to existing search path */
    if (opt.add->answer) {
	const char *oldname;

	Path[0] = '\0';

	/* read existing mapsets from SEARCH_PATH */
	for (n = 0; (oldname = G__mapset_name(n)); n++) {
	    strcat(Path, oldname);
	    strcat(Path, " ");
	}

	/* fetch and add new mapsets from param list */
	for (ptr = opt.add->answers; *ptr != NULL; ptr++) {
	    char *mapset;

	    mapset = *ptr;

	    if (G_is_mapset_in_search_path(mapset))
		continue;
	    
	    if (G__mapset_permissions(mapset) < 0)
		G_fatal_error(_("Mapset <%s> not found"), mapset);
	    else
		G_verbose_message(_("Mapset <%s> added to search path"),
				  mapset);

	    nchoices++;
	    strcat(Path, mapset);
	    strcat(Path, " ");
	}
    }

    /* remove from existing search path */
    if (opt.remove->answer) {
	const char *oldname;

	Path[0] = '\0';

	/* read existing mapsets from SEARCH_PATH */
	for (n = 0; (oldname = G__mapset_name(n)); n++) {
	    int found = 0;

	    for (ptr = opt.remove->answers; *ptr; ptr++)
		if (strcmp(oldname, *ptr) == 0)
		    found = 1;

	    if (found) {
		G_verbose_message(_("Mapset <%s> removed from search path"),
				  oldname);
		continue;
	    }

	    nchoices++;
	    strcat(Path, oldname);
	    strcat(Path, " ");
	}
    }

    /* stuffem sets nchoices */

    if (nchoices == 0) {
	if (opt.print->answer)
	    display_mapset_path(opt.fs->answer);

	exit(EXIT_SUCCESS);
    }

    /* note I'm assuming that mapsets cannot have ' 's in them */
    tokens = G_tokenize(Path, " ");

    fp = G_fopen_new("", "SEARCH_PATH");
    if (!fp)
	G_fatal_error(_("Cannot open SEARCH_PATH for write"));

    cur_mapset = G_mapset();

    /*
     * make sure current mapset is specified in the list
     * if not add it to the head of the list
     */

    skip = 0;
    for (n = 0; n < nchoices; n++)
	if (strcmp(cur_mapset, tokens[n]) == 0) {
	    skip = 1;
	    break;
	}
    if (!skip) {
	fprintf(fp, "%s\n", cur_mapset);
    }

    /*
     * output the list, removing duplicates
     */

    no_tokens = G_number_of_tokens(tokens);

    for (n = 0; n < no_tokens; n++) {
	skip = 0;
	for (i = n; i < no_tokens; i++) {
	    if (i != n) {
		if (strcmp(tokens[i], tokens[n]) == 0)
		    skip = 1;
	    }
	}

	if (!skip)
	    fprintf(fp, "%s\n", tokens[n]);
    }

    fclose(fp);
    G_free_tokens(tokens);

    exit(EXIT_SUCCESS);
}
