
/****************************************************************************
 *
 * MODULE:       g.mapsets
 * AUTHOR(S):    Michael Shapiro (CERL), Greg Koerper (ManTech Environmental
 *                    Technology) (original contributors), 
 *               Glynn Clements <glynn gclements.plus.com>
 *               Hamish Bowman <hamish_nospam yahoo.com>, 
 *               Markus Neteler <neteler itc.it>, 
 *               Moritz Lennert <mlennert club.worldonline.be>
 * PURPOSE:      set current mapset path
 * COPYRIGHT:    (C) 1994-2008 by the GRASS Development Team
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
    struct Option *opt1, *opt2, *opt3;
    struct Flag *print;
    struct Flag *list;
    struct Flag *tcl;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("general, settings");
    module->description =
	_("Modifies the user's current mapset "
	  "search path, affecting the user's access to data existing "
	  "under the other GRASS mapsets in the current location.");

    opt1 = G_define_option();
    opt1->key = "mapset";
    opt1->type = TYPE_STRING;
    opt1->required = NO;
    opt1->multiple = YES;
    opt1->description = _("Name(s) of existing mapset(s)");

    opt2 = G_define_option();
    opt2->key = "addmapset";
    opt2->type = TYPE_STRING;
    opt2->required = NO;
    opt2->multiple = YES;
    opt2->description =
	_("Name(s) of existing mapset(s) to add to search list");

    opt3 = G_define_option();
    opt3->key = "removemapset";
    opt3->type = TYPE_STRING;
    opt3->required = NO;
    opt3->multiple = YES;
    opt3->description =
	_("Name(s) of existing mapset(s) to remove from search list");

    list = G_define_flag();
    list->key = 'l';
    list->description = _("List all available mapsets");

    print = G_define_flag();
    print->key = 'p';
    print->description = _("Print current mapset search path");

    tcl = G_define_flag();
    tcl->key = 's';
    tcl->description = _("Show mapset selection dialog");

    Path[0] = '\0';
    nchoices = 0;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (list->answer) {
	get_available_mapsets();
	display_available_mapsets(0);
    }

    if (tcl->answer) {
	sprintf(path, "%s/etc/g.mapsets.tcl", G_gisbase());
	G_spawn(path, "g.mapsets.tcl", NULL);
    }

    if (opt1->answer) {
	for (ptr = opt1->answers; *ptr != NULL; ptr++) {
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
    if (opt2->answer) {
	const char *oldname;

	Path[0] = '\0';

	/* read existing mapsets from SEARCH_PATH */
	for (n = 0; (oldname = G__mapset_name(n)); n++) {
	    strcat(Path, oldname);
	    strcat(Path, " ");
	}

	/* fetch and add new mapsets from param list */
	for (ptr = opt2->answers; *ptr != NULL; ptr++) {
	    char *mapset;

	    mapset = *ptr;
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
    if (opt3->answer) {
	const char *oldname;

	Path[0] = '\0';

	/* read existing mapsets from SEARCH_PATH */
	for (n = 0; (oldname = G__mapset_name(n)); n++) {
	    int found = 0;

	    for (ptr = opt3->answers; *ptr; ptr++)
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
	goto DISPLAY;
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

  DISPLAY:
    if (print->answer)
	display_mapset_path(0);

    exit(EXIT_SUCCESS);
}
