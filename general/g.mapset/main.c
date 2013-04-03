
/****************************************************************
 *
 * MODULE:       g.mapset
 * 
 * AUTHOR(S):    Radim Blazek
 *               Joel Pitt, joel.pitt@gmail.com
 *               
 * PURPOSE:      Change current mapset, optionally adding it
 *               if the mapset does not exist.
 *               
 * COPYRIGHT:    (C) 2004, 2010-2011 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/spawn.h>

int main(int argc, char *argv[])
{
    int ret;
    struct GModule *module;
    struct {
	struct Option *gisdbase, *location, *mapset;
    } opt;
    struct {
	struct Flag *add, *list, *curr;
    } flag;
    const char *gisdbase_old, *location_old, *mapset_old;
    const char *gisdbase_new, *location_new, *mapset_new;
    const char *gis_lock;
    char *mapset_old_path, *mapset_new_path;
    char *lock_prog;
    const char *shell;
    char path[GPATH_MAX];

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("settings"));
    module->label = _("Changes/reports current mapset.");
    module->description = _("Optionally create new mapset or list available mapsets in given location.");
    
    opt.mapset = G_define_standard_option(G_OPT_M_MAPSET);
    opt.mapset->required = YES;
    opt.mapset->description = _("Name of mapset where to switch");
    opt.mapset->guisection = _("Settings");
    opt.mapset->gisprompt = "new,mapset,mapset";

    opt.location = G_define_option();
    opt.location->key = "location";
    opt.location->type = TYPE_STRING;
    opt.location->required = NO;
    opt.location->multiple = NO;
    opt.location->key_desc = "name";
    opt.location->description = _("Location name (not location path)");
    opt.location->guisection = _("Settings");

    opt.gisdbase = G_define_option();
    opt.gisdbase->key = "gisdbase";
    opt.gisdbase->type = TYPE_STRING;
    opt.gisdbase->required = NO;
    opt.gisdbase->multiple = NO;
    opt.gisdbase->key_desc = "path";
    opt.gisdbase->label = _("GIS data directory");
    opt.gisdbase->description = _("Full path to the directory where the new location is");
    opt.gisdbase->guisection = _("Settings");

    flag.add = G_define_flag();
    flag.add->key = 'c';
    flag.add->description = _("Create mapset if it doesn't exist");
    flag.add->answer = FALSE;
    flag.add->guisection = _("Create");

    flag.list = G_define_flag();
    flag.list->key = 'l';
    flag.list->suppress_required = YES;
    flag.list->description = _("List available mapsets and exit");
    flag.list->guisection = _("Print");
 
    flag.curr = G_define_flag();
    flag.curr->key = 'p';
    flag.curr->suppress_required = YES;
    flag.curr->description = _("Print current mapset and exit");
    flag.curr->guisection = _("Print");
   
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Store original values */
    gisdbase_old = G__getenv("GISDBASE");
    location_old = G__getenv("LOCATION_NAME");
    mapset_old = G__getenv("MAPSET");

    if (flag.curr->answer) {
	fprintf(stdout, "%s\n", mapset_old);
	exit(EXIT_SUCCESS);
    }
    
    G_asprintf(&mapset_old_path, "%s/%s/%s", gisdbase_old, location_old,
	       mapset_old);

    /* New values */
    if (opt.gisdbase->answer)
	gisdbase_new = opt.gisdbase->answer;
    else
	gisdbase_new = gisdbase_old;

    if (opt.location->answer)
	location_new = opt.location->answer;
    else
	location_new = location_old;

    if (flag.list->answer) {
	char **ms;
	int nmapsets;

	G__setenv("LOCATION_NAME", location_new);
	G__setenv("GISDBASE", gisdbase_new);

	ms = G_available_mapsets();

	for (nmapsets = 0; ms[nmapsets]; nmapsets++) {
	    if (G__mapset_permissions(ms[nmapsets]) > 0) {
		fprintf(stdout, "%s ", ms[nmapsets]);
	    }
	}
	fprintf(stdout, "\n");

	exit(EXIT_SUCCESS);
    }

    mapset_new = opt.mapset->answer;
    G_asprintf(&mapset_new_path, "%s/%s/%s", gisdbase_new, location_new,
	       mapset_new);

    /* TODO: this should be checked better (repeated '/' etc.) */
    if (strcmp(mapset_old_path, mapset_new_path) == 0) {
	G_warning(_("<%s> is already the current mapset"), mapset_new);
	exit(EXIT_SUCCESS);
    }
    /* Check if the mapset exists and user is owner */
    G_debug(2, "check : %s", mapset_new_path);

    ret = G__mapset_permissions2(gisdbase_new, location_new, mapset_new);
    switch (ret) {
    case 0:
	G_fatal_error(_("You don't have permission to use the mapset <%s>"),
	              mapset_new);
	break;
    case -1:
	if (flag.add->answer == TRUE) {
	    G_debug(2, "Mapset <%s> doesn't exist, attempting to create it",
		    mapset_new);
	    if (G_make_mapset(gisdbase_new, location_new, mapset_new) != 0)
                G_fatal_error(_("Unable to create new mapset <%s>"), mapset_new);
	}
	else
	    G_fatal_error(_("Mapset <%s> does not exist. Use -c flag to create it."),
	                  mapset_new);
	break;
    default:
	break;
    }

    /* Check if the mapset is in use */
    gis_lock = getenv("GIS_LOCK");
    if (!gis_lock)
	G_fatal_error(_("Unable to read GIS_LOCK environment variable"));

    G_asprintf(&lock_prog, "%s/etc/lock", G_gisbase());

    sprintf(path, "%s/.gislock", mapset_new_path);
    G_debug(2, path);
    
    ret = G_spawn(lock_prog, lock_prog, path, gis_lock, NULL);
    G_debug(2, "lock result = %d", ret);
    G_free(lock_prog);

    /* Warning: the value returned by system() is not that returned by exit() in executed program
     *          e.g. exit(1) -> 256 (multiplied by 256) */
    if (ret != 0) {
	/* .gislock does not exist */
	if (access(path, F_OK) != 0)
	    G_fatal_error(_("Lock file of mapset <%s> cannot be checked"),
			   mapset_new);
	else
	    G_fatal_error(_("There appears to be an active GRASS session in selected mapset <%s>"),
		          mapset_new);
    }

    /* Clean temporary directory */
    sprintf(path, "%s/etc/clean_temp", G_gisbase());
    G_verbose_message(_("Cleaning up temporary files..."));
    G_spawn(path, "clean_temp", NULL);

    /* Reset variables */
    G_setenv("GISDBASE", gisdbase_new);
    G_setenv("LOCATION_NAME", location_new);
    G_setenv("MAPSET", mapset_new);

    /* Remove old lock */
    sprintf(path, "%s/.gislock", mapset_old_path);
    remove(path);

    G_free(mapset_old_path);

    G_important_message(_("Your shell continues to use the history "
			  "for the old mapset"));

    if ((shell = getenv("SHELL"))) {
	if (strstr(shell, "bash")) {
	    G_important_message(_("You can switch the history by commands:\n"
				  "history -w; history -r %s/.bash_history; HISTFILE=%s/.bash_history"),
				mapset_new_path, mapset_new_path);
	}
	else if (strstr(shell, "tcsh")) {
	    G_important_message(_("You can switch the history by commands:\n"
				  "history -S; history -L %s/.history; setenv histfile=%s/.history"),
				mapset_new_path, mapset_new_path);
	}
    }

    G_verbose_message(_("Your current mapset is <%s>"), mapset_new);
    
    G_free(mapset_new_path);

    return (EXIT_SUCCESS);
}
