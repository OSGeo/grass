
/****************************************************************************
 *
 * MODULE:       i.photo.target
 * AUTHOR(S):    Mike Baba,  DBA Systems, Inc. (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>
 *               Hamish Bowman
 *
 * PURPOSE:      Select target project and subproject
 * COPYRIGHT:    (C) 1999-2017 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "orthophoto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *group_opt;
    struct Option *project_opt;
    struct Option *subproject_opt;

    /* current project and subproject of that group */
    char project[GMAPSET_MAX];
    char subproject[GMAPSET_MAX];
    char group[GNAME_MAX];
    /* newly defined project and maspet */
    char target_project[GMAPSET_MAX];
    char target_subproject[GMAPSET_MAX];
    int stat;
    struct Cell_head target_window;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("orthorectify"));
    module->description =
	_("Select or modify the imagery group target.");

    group_opt = G_define_standard_option(G_OPT_I_GROUP);
    group_opt->description =
	_("Name of imagery group for ortho-rectification");

    project_opt = G_define_standard_option(G_OPT_M_LOCATION);
    project_opt->key = "target_project";
    project_opt->required = YES;
    project_opt->description =
	_("Name of target project for ortho-rectification");

    subproject_opt = G_define_standard_option(G_OPT_M_MAPSET);
    subproject_opt->key = "subproject_project";
    subproject_opt->required = YES;
    subproject_opt->description =
	_("Name of target subproject for ortho-rectification");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    strcpy(group, group_opt->answer);
    strcpy(target_project, project_opt->answer);
    strcpy(target_subproject, subproject_opt->answer);

    I_get_target(group, project, subproject);
    G_create_alt_env();
    G_setenv_nogisrc("LOCATION_NAME", target_project);
    stat = G_subproject_permissions(target_subproject);
    if (stat != 1) {
	G_fatal_error(_("Unable to access target project/subproject %s/%s"),
	              target_project, target_subproject);
    }

    G_setenv_nogisrc("MAPSET", target_subproject);
    G_get_window(&target_window);
    if (target_window.proj == PROJECTION_XY)
	G_fatal_error(_("Target projects with XY (unreferenced) are not supported"));
    else if (target_window.proj == PROJECTION_LL)
	G_fatal_error(_("Target projects with lon/lat are not supported"));

    G_switch_env();
    I_put_target(group, target_project, target_subproject);

    G_message(_("Group [%s] targeted for project [%s], subproject [%s]"),
	    group, target_project, target_subproject);

    exit(EXIT_SUCCESS);
}
