
/****************************************************************************
 *
 * MODULE:       i.photo.elev
 * AUTHOR(S):    Mike Baba (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>
 *               Hamish Bowman
 *               Markus Metz
 *
 * PURPOSE:      Select the elevation model
 * COPYRIGHT:    (C) 1999-2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/* read the target for the block and cast it into the alternate GRASS env */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "orthophoto.h"

static int which_env;

int select_current_env(void);
int select_target_env(void);

int main(int argc, char *argv[])
{

    struct GModule *module;
    struct Option *group_opt, *loc_opt, *subproject_opt, *elev_opt;
    struct Option *math_opt, *unit_opt, *nd_opt;
    struct Flag *list_flag, *print_flag;

    char project[GMAPSET_MAX];
    char subproject[GMAPSET_MAX];
    char group[GNAME_MAX];

    char *elev_layer;
    char *subproject_elev, *subproject_elev_old;
    char *project_elev, *project_elev_old;
    char *math_exp;
    char *units;
    char *nd;

    char buf[GPATH_MAX];
    int stat;
    int overwrite;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("orthorectify"));
    module->description =
	_("Select or modify the target elevation model.");

    group_opt = G_define_standard_option(G_OPT_I_GROUP);
    group_opt->description =
	_("Name of imagery group for ortho-rectification");

    loc_opt = G_define_standard_option(G_OPT_M_LOCATION);
    loc_opt->required = NO;
    loc_opt->description =
	_("Name of the target project");

    subproject_opt = G_define_standard_option(G_OPT_M_MAPSET);
    subproject_opt->required = NO;
    subproject_opt->description =
	_("Name of the target subproject");

    elev_opt = G_define_standard_option(G_OPT_R_ELEV);
    elev_opt->required = NO;
    elev_opt->description =
	_("Name of elevation map to use for ortho-rectification");

    math_opt = G_define_standard_option(G_OPT_M_NULL_VALUE);
    math_opt->key = "math_expression";
    math_opt->required = NO;
    math_opt->description =
	_("Math expression to convert to real elevation");

    unit_opt = G_define_standard_option(G_OPT_M_UNITS);
    unit_opt->required = NO;
    unit_opt->description =
	_("Unit of the elevation map");

    nd_opt = G_define_standard_option(G_OPT_M_NULL_VALUE);
    nd_opt->required = NO;
    nd_opt->description =
	_("No data value");

    print_flag = G_define_flag();
    print_flag->key = 'p';
    print_flag->description =
	_("Print currently selected elevation map and exit");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    elev_layer = (char *)G_malloc(GNAME_MAX * sizeof(char));
    subproject_elev = (char *)G_malloc(GMAPSET_MAX * sizeof(char));
    subproject_elev_old = (char *)G_malloc(GMAPSET_MAX * sizeof(char));
    project_elev = (char *)G_malloc(80 * sizeof(char));
    project_elev_old = (char *)G_malloc(80 * sizeof(char));
    math_exp = (char *)G_malloc(80 * sizeof(char));
    units = (char *)G_malloc(80 * sizeof(char));
    nd = (char *)G_malloc(80 * sizeof(char));

    *elev_layer = 0;
    *subproject_elev = 0;
    *subproject_elev_old = 0;
    *project_elev = 0;
    *project_elev_old = 0;
    *math_exp = 0;
    *units = 0;
    *nd = 0;

    strcpy(group, group_opt->answer);
    if (loc_opt->answer)
    	strcpy(project_elev, loc_opt->answer);
    if (subproject_opt->answer)
    	strcpy(subproject_elev, subproject_opt->answer);
    /*if(elev_opt->answer)
    	strcpy(elev_layer, elev_opt->answer);*/
    if (math_opt->answer)
    	strcpy(math_exp, math_opt->answer);
    if (unit_opt->answer)
    	strcpy(units, unit_opt->answer);
    if (nd_opt->answer)
    	strcpy(nd, nd_opt->answer);
	
    if (!I_get_target(group, project, subproject)) {
	G_fatal_error(_("Please select a target for group [%s] first"), group);
    }

    sprintf(buf, "%s/%s", G_gisdbase(), project);
    if (access(buf, 0) != 0) {
	G_fatal_error(_("Target project [%s] not found\n"), project);
    }

    /*Report the contents of the ELEVATION file as in the GROUP */
    if (print_flag->answer) {
	/*If the content is empty report an error */
	if (!I_get_group_elev(group, elev_layer, subproject_elev, project_elev, math_exp, units, nd)) {
		G_fatal_error(_("Cannot find default elevation map for target in group [%s]"),group);
	}
	/*If there is a content, print it */
	else {
	    fprintf(stdout, "map:\t\t\t%s\n",elev_layer);
	    fprintf(stdout, "subproject:\t\t\t%s\n",subproject_elev);
	    fprintf(stdout, "project:\t\t%s\n",project_elev);
	    fprintf(stdout, "math expression:\t%s\n",math_exp);
	    fprintf(stdout, "units:\t\t\t%s\n",units);
	    fprintf(stdout, "nodata value:\t\t%s\n",nd);
	    exit(EXIT_SUCCESS);
	}    
    }

    /*Creating a Target environment*/
    G_create_alt_env();
    /*Set an alternate Project */
    G_setenv_nogisrc("LOCATION_NAME", project);
    /*Check for permissions in subproject */
    stat = G_subproject_permissions(subproject);
    /*We have permissions on the subproject */
    if (stat > 0) {
	/*Set the alternate subproject */
	G_setenv_nogisrc("MAPSET", subproject);
	/*Make an alternate search path */
	G_create_alt_search_path();
	/*Switch from current to alternate environment */
	G_switch_env();
	/*Switch to alternate search path */
	G_switch_search_path();
	/*Source Env = 0 ; Target Env = 1 */
	which_env = 0;
        /*Select the target environment */
	select_target_env();
	/*Start working on the Target subproject */
        if (!elev_opt->answer) {
	    /* select current project */
	    select_current_env();
	    G_fatal_error(_("Elevation map name is missing. Please set '%s' option"),
	        elev_opt->key);
	}
	
	/* return to current Project/subproject to write in the group file */
	select_current_env();
	
	/* load information from the ELEVATION file in the GROUP */
	if (I_find_group_elev_file(group)) {
	    I_get_group_elev(group, elev_layer, subproject_elev_old, project_elev_old,
	                     math_exp, units, nd);
	    if (*project_elev == 0)
		strcpy(project_elev, project_elev_old);
	    if (*subproject_elev == 0)
		strcpy(subproject_elev, subproject_elev_old);
	}
	/* if project and/or subproject of elevation are not set, use target */
	if (*subproject_elev == 0)
	    strcpy(subproject_elev, subproject);
	if (*project_elev == 0)
	    strcpy(project_elev, project);

	/* select target project */
	select_target_env();
	/* elevation map exists in target ? */
	if (G_find_raster2(elev_opt->answer, subproject_elev) == NULL) {
            /* select current project */
	    select_current_env();
	    G_fatal_error(_("Raster map <%s> not found"), elev_opt->answer);
	}
	/* select current project */
	select_current_env();

	/* Modify ELEVATION file in source GROUP */
	I_put_group_elev(group, elev_opt->answer, subproject_elev, project_elev, 
			 math_exp, units, nd);

        G_message(_("Group [%s] in project [%s] subproject [%s] now uses elevation map [%s]"),
	          group, G_project(), G_subproject(), elev_opt->answer);
    }
    else {
	G_fatal_error(_("Subproject [%s] in target project [%s] - %s "),
                  subproject, project,
		  stat == 0 ? _("permission denied\n") : _("not found\n"));
    }

    return(EXIT_SUCCESS);
}


int select_current_env(void)
{
    if (which_env != 0) {
	G_switch_env();
	G_switch_search_path();
	which_env = 0;
    }

    return 0;
}

int select_target_env(void)
{
    if (which_env != 1) {
	G_switch_env();
	G_switch_search_path();
	which_env = 1;
    }

    return 0;
}
