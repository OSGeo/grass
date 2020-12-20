
/****************************************************************
 *
 * MODULE:       g.subproject
 * 
 * AUTHOR(S):    Radim Blazek
 *               Joel Pitt, joel.pitt@gmail.com
 *               
 * PURPOSE:      Change current subproject, optionally adding it
 *               if the subproject does not exist.
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
	struct Option *gisdbase, *project, *subproject;
    } opt;
    struct {
	struct Flag *add, *list, *curr;
    } flag;
    const char *gisdbase_old, *project_old, *subproject_old;
    const char *gisdbase_new, *project_new, *subproject_new;
    const char *gis_lock;
    char *subproject_old_path, *subproject_new_path;
    char *lock_prog;
    const char *shell;
    char path[GPATH_MAX];

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("settings"));
    module->label = _("Changes/reports current subproject.");
    module->description = _("Optionally create new subproject or list available subprojects in given project.");
    
    opt.subproject = G_define_standard_option(G_OPT_M_MAPSET);
    opt.subproject->required = YES;
    opt.subproject->description = _("Name of subproject where to switch");
    opt.subproject->guisection = _("Subproject");
    opt.subproject->gisprompt = "new,subproject,subproject";

    opt.project = G_define_standard_option(G_OPT_M_LOCATION);
    opt.project->guisection = _("Subproject");

    opt.gisdbase = G_define_standard_option(G_OPT_M_DBASE);
    opt.gisdbase->guisection = _("Subproject");

    flag.add = G_define_flag();
    flag.add->key = 'c';
    flag.add->description = _("Create subproject if it doesn't exist");
    flag.add->answer = FALSE;
    flag.add->guisection = _("Create");

    flag.list = G_define_flag();
    flag.list->key = 'l';
    flag.list->suppress_required = YES;
    flag.list->description = _("List available subprojects and exit");
    flag.list->guisection = _("Print");
 
    flag.curr = G_define_flag();
    flag.curr->key = 'p';
    flag.curr->suppress_required = YES;
    flag.curr->description = _("Print current subproject and exit");
    flag.curr->guisection = _("Print");
   
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Store original values */
    gisdbase_old = G_getenv_nofatal("GISDBASE");
    project_old = G_getenv_nofatal("LOCATION_NAME");
    subproject_old = G_getenv_nofatal("MAPSET");

    if (flag.curr->answer) {
	fprintf(stdout, "%s\n", subproject_old);
	exit(EXIT_SUCCESS);
    }
    
    G_asprintf(&subproject_old_path, "%s/%s/%s", gisdbase_old, project_old,
	       subproject_old);

    /* New values */
    if (opt.gisdbase->answer)
	gisdbase_new = opt.gisdbase->answer;
    else
	gisdbase_new = gisdbase_old;

    if (opt.project->answer)
	project_new = opt.project->answer;
    else
	project_new = project_old;

    if (flag.list->answer) {
	char **ms;
	int nsubprojects;

	G_setenv_nogisrc("LOCATION_NAME", project_new);
	G_setenv_nogisrc("GISDBASE", gisdbase_new);

	ms = G_get_available_subprojects();

	for (nsubprojects = 0; ms[nsubprojects]; nsubprojects++) {
	    if (G_subproject_permissions(ms[nsubprojects]) > 0) {
		fprintf(stdout, "%s ", ms[nsubprojects]);
	    }
	}
	fprintf(stdout, "\n");

	exit(EXIT_SUCCESS);
    }

    subproject_new = opt.subproject->answer;
    G_asprintf(&subproject_new_path, "%s/%s/%s", gisdbase_new, project_new,
	       subproject_new);

    /* TODO: this should be checked better (repeated '/' etc.) */
    if (strcmp(subproject_old_path, subproject_new_path) == 0) {
	G_warning(_("<%s> is already the current subproject"), subproject_new);
	exit(EXIT_SUCCESS);
    }
    /* Check if the subproject exists and user is owner */
    G_debug(2, "check : %s", subproject_new_path);

    ret = G_subproject_permissions2(gisdbase_new, project_new, subproject_new);
    switch (ret) {
    case 0:
	G_fatal_error(_("You don't have permission to use the subproject <%s>"),
	              subproject_new);
	break;
    case -1:
	if (flag.add->answer == TRUE) {
	    G_debug(2, "Subproject <%s> doesn't exist, attempting to create it",
		    subproject_new);
	    if (G_make_subproject(gisdbase_new, project_new, subproject_new) != 0)
                G_fatal_error(_("Unable to create new subproject <%s>"), subproject_new);
	}
	else
	    G_fatal_error(_("Subproject <%s> does not exist. Use -c flag to create it."),
	                  subproject_new);
	break;
    default:
	break;
    }

    /* Check if the subproject is in use */
    gis_lock = getenv("GIS_LOCK");
    if (!gis_lock)
	G_fatal_error(_("Unable to read GIS_LOCK environment variable"));

    G_asprintf(&lock_prog, "%s/etc/lock", G_gisbase());

    sprintf(path, "%s/.gislock", subproject_new_path);
    G_debug(2, "%s", path);
    
    ret = G_spawn(lock_prog, lock_prog, path, gis_lock, NULL);
    G_debug(2, "lock result = %d", ret);
    G_free(lock_prog);

    /* Warning: the value returned by system() is not that returned by exit() in executed program
     *          e.g. exit(1) -> 256 (multiplied by 256) */
    if (ret != 0) {
	/* .gislock does not exist */
	if (access(path, F_OK) != 0)
	    G_fatal_error(_("Lock file of subproject <%s> cannot be checked"),
			   subproject_new);
	else
	    G_fatal_error(_("There appears to be an active GRASS session in selected subproject <%s>"),
		          subproject_new);
    }

    /* Clean temporary directory */
    sprintf(path, "%s/etc/clean_temp", G_gisbase());
    G_verbose_message(_("Cleaning up temporary files..."));
    G_spawn(path, "clean_temp", NULL);

    /* Reset variables */
    G_setenv("GISDBASE", gisdbase_new);
    G_setenv("LOCATION_NAME", project_new);
    G_setenv("MAPSET", subproject_new);

    /* Remove old lock */
    sprintf(path, "%s/.gislock", subproject_old_path);
    remove(path);

    G_free(subproject_old_path);

    shell = getenv("SHELL");
    /* For bash and zsh, we support switching of history, tcsh not (yet). */
    if (shell && (strstr(shell, "bash") || strstr(shell, "zsh"))) {
	G_important_message(_("Subproject switched."));
    }
    else {
	G_important_message(_("Subproject switched. Your shell continues "
			      "to use the history for the old subproject"));
    }
    if (shell && strstr(shell, "tcsh")) {
	G_important_message(_("You can switch the history by commands:\n"
			      "history -S; history -L %s/.history; setenv histfile=%s/.history"),
			      subproject_new_path, subproject_new_path);
    }

    G_verbose_message(_("Your current subproject is <%s>"), subproject_new);
    
    G_free(subproject_new_path);

    return (EXIT_SUCCESS);
}
