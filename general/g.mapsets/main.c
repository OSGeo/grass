
/****************************************************************************
 *
 * MODULE:       g.mapsets
 * AUTHOR(S):    Michael Shapiro (CERL), Greg Koerper (ManTech Environmental
 *                    Technology) (original contributors), 
 *               Glynn Clements <glynn gclements.plus.com>
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Markus Neteler <neteler itc.it>, 
 *               Moritz Lennert <mlennert club.worldonline.be>,
 *               Martin Landa <landa.martin gmail.com>,
 *               Huidae Cho <grass4u gmail.com>
 * PURPOSE:      set current mapset path
 * COPYRIGHT:    (C) 1994-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#define _MAIN_C_
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/spawn.h>
#include <grass/glocale.h>
#include "local_proto.h"

static void append_mapset(char **, const char *);

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
    char *Path;
    int nchoices;

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
    G_add_keyword(_("general"));
    G_add_keyword(_("settings"));
    G_add_keyword(_("search path"));
    module->label = _("Modifies the user's current mapset search path.");
    module->description = _("Affects the user's access to data existing "
			    "under the other mapsets in the current location.");

    opt.mapset = G_define_option();
    opt.mapset->key = "mapset";
    opt.mapset->type = TYPE_STRING;
    opt.mapset->required = NO;
    opt.mapset->multiple = YES;
    opt.mapset->description = _("Name(s) of existing mapset(s)");
    opt.mapset->guisection = _("Search path");
    
    opt.add = G_define_option();
    opt.add->key = "addmapset";
    opt.add->type = TYPE_STRING;
    opt.add->required = NO;
    opt.add->multiple = YES;
    opt.add->description =
	_("Name(s) of existing mapset(s) to add to search path");
    opt.add->guisection = _("Search path");

    opt.remove = G_define_option();
    opt.remove->key = "removemapset";
    opt.remove->type = TYPE_STRING;
    opt.remove->required = NO;
    opt.remove->multiple = YES;
    opt.remove->description =
	_("Name(s) of existing mapset(s) to remove from search path");
    opt.remove->guisection = _("Search path");

    opt.fs = G_define_standard_option(G_OPT_F_SEP);
    opt.fs->answer = "space";
    
    opt.list = G_define_flag();
    opt.list->key = 'l';
    opt.list->description = _("List all available mapsets");
    opt.list->guisection = _("Print");

    opt.print = G_define_flag();
    opt.print->key = 'p';
    opt.print->description = _("Print current mapset search path");
    opt.print->guisection = _("Print");

    opt.dialog = G_define_flag();
    opt.dialog->key = 's';
    opt.dialog->description = _("Show mapset selection dialog");

    Path = NULL;
    nmapsets = 0;
    nchoices = 0;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (opt.list->answer) {
	get_available_mapsets();
	list_available_mapsets(opt.fs->answer);
    }

    if (opt.dialog->answer) {
	sprintf(path, "%s/etc/gui/scripts/g.mapsets_picker.py", G_gisbase());
	G_spawn(getenv("GRASS_PYTHON"), "g.mapsets_picker.py", path, NULL);
    }

    if (opt.mapset->answer) {
	for (ptr = opt.mapset->answers; *ptr != NULL; ptr++) {
	    const char *mapset;

	    mapset = *ptr;
	    if (G__mapset_permissions(mapset) < 0)
		G_fatal_error(_("Mapset <%s> not found"), mapset);
	    nchoices++;
	    append_mapset(&Path, mapset);
	}
    }

    /* add to existing search path */
    if (opt.add->answer) {
	const char *oldname;

	if (Path) {
	    G_free(Path);
	    Path = NULL;
	}

	/* read existing mapsets from SEARCH_PATH */
	for (n = 0; (oldname = G__mapset_name(n)); n++)
	    append_mapset(&Path, oldname);

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
	    append_mapset(&Path, mapset);
	}
    }

    /* remove from existing search path */
    if (opt.remove->answer) {
	const char *oldname;

	if (Path) {
	    G_free(Path);
	    Path = NULL;
	}

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
	    append_mapset(&Path, oldname);
	}
    }

    /* stuffem sets nchoices */

    if (nchoices == 0) {
	if (opt.print->answer)
	    list_accessible_mapsets(opt.fs->answer);

	if (Path)
	    G_free(Path);

	if (nmapsets) {
	    for(nmapsets--; nmapsets >= 0; nmapsets--)
		G_free(mapset_name[nmapsets]);
	    G_free(mapset_name);
	}

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

    if (Path)
	G_free(Path);

    if (nmapsets) {
	for(nmapsets--; nmapsets >= 0; nmapsets--)
	    G_free(mapset_name[nmapsets]);
	G_free(mapset_name);
    }

    exit(EXIT_SUCCESS);
}

static void append_mapset(char **path, const char *mapset)
{
    int len = (*path == NULL ? 0 : strlen(*path));

    *path = (char *)G_realloc(*path, len + strlen(mapset) + 2);
    if (!len)
        *path[0] = '\0';
    strcat(*path, mapset);
    strcat(*path, " ");
    return;
}
