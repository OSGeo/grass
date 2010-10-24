
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
 * COPYRIGHT:    (C) 2004, 2010 by the GRASS Development Team
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
    struct Option *gisdbase_opt, *location_opt, *mapset_opt;
    struct Flag *f_add, *f_list;
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
    module->label = _("Changes current mapset.");
    module->description = _("Optionally create new mapset or list available mapsets in given location.");
    
    mapset_opt = G_define_option();
    mapset_opt->key = "mapset";
    mapset_opt->type = TYPE_STRING;
    mapset_opt->required = NO;
    mapset_opt->multiple = NO;
    mapset_opt->description = _("Name of mapset where to switch");
    mapset_opt->guisection = _("Settings");

    location_opt = G_define_option();
    location_opt->key = "location";
    location_opt->type = TYPE_STRING;
    location_opt->required = NO;
    location_opt->multiple = NO;
    location_opt->description = _("Location name (not location path)");
    location_opt->guisection = _("Settings");

    gisdbase_opt = G_define_option();
    gisdbase_opt->key = "gisdbase";
    gisdbase_opt->type = TYPE_STRING;
    gisdbase_opt->required = NO;
    gisdbase_opt->multiple = NO;
    gisdbase_opt->key_desc = "path";
    gisdbase_opt->description =
	_("GIS data directory (full path to the directory where the new location is)");
    gisdbase_opt->guisection = _("Settings");

    f_add = G_define_flag();
    f_add->key = 'c';
    f_add->description = _("Create mapset if it doesn't exist");
    f_add->answer = FALSE;
    f_add->guisection = _("Create");

    f_list = G_define_flag();
    f_list->key = 'l';
    f_list->description = _("List available mapsets");
    f_list->guisection = _("Print");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (!mapset_opt->answer && !f_list->answer)
	G_fatal_error(_("Either mapset= or -l must be used"));

    /* Store original values */
    gisdbase_old = G__getenv("GISDBASE");
    location_old = G__getenv("LOCATION_NAME");
    mapset_old = G__getenv("MAPSET");
    G_asprintf(&mapset_old_path, "%s/%s/%s", gisdbase_old, location_old,
	       mapset_old);

    /* New values */
    if (gisdbase_opt->answer)
	gisdbase_new = gisdbase_opt->answer;
    else
	gisdbase_new = gisdbase_old;

    if (location_opt->answer)
	location_new = location_opt->answer;
    else
	location_new = location_old;

    if (f_list->answer) {
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

    mapset_new = mapset_opt->answer;
    G_asprintf(&mapset_new_path, "%s/%s/%s", gisdbase_new, location_new,
	       mapset_new);

    /* TODO: this should be checked better (repeated '/' etc.) */
    if (strcmp(mapset_old_path, mapset_new_path) == 0)
	G_fatal_error(_("<%s> is already the current mapset"), mapset_new);

    /* Check if the mapset exists and user is owner */
    G_debug(2, "check : %s", mapset_new_path);

    ret = G__mapset_permissions2(gisdbase_new, location_new, mapset_new);
    switch (ret) {
    case 0:
	G_fatal_error(_("You don't have permission to use this mapset"));
	break;
    case -1:
	if (f_add->answer == TRUE) {
	    G_debug(2, "Mapset %s doesn't exist, attempting to create it",
		    mapset_new);
	    G_make_mapset(gisdbase_new, location_new, mapset_new);
	}
	else
	    G_fatal_error(_("The mapset does not exist. Use -c flag to create it."));
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
    if (ret != 0)
	G_fatal_error(_("%s is currently running GRASS in selected mapset or lock file cannot be checked"),
		      G_whoami());

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

    G_important_message(_("Your shell continues to use the history for the old mapset"));

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

    G_message(_("Your current mapset is <%s>"), mapset_new);
    
    G_free(mapset_new_path);

    return (EXIT_SUCCESS);
}
