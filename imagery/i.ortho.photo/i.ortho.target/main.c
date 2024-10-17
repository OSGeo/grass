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
 * PURPOSE:      Select target location and mapset
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
    struct Option *location_opt;
    struct Option *mapset_opt;

    /* current location and mapset of that group */
    char location[GMAPSET_MAX];
    char mapset[GMAPSET_MAX];
    char group[GNAME_MAX];

    /* newly defined location and maspet */
    char target_location[GMAPSET_MAX];
    char target_mapset[GMAPSET_MAX];
    int stat;
    struct Cell_head target_window;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("orthorectify"));
    module->description = _("Select or modify the imagery group target.");

    group_opt = G_define_standard_option(G_OPT_I_GROUP);
    group_opt->description = _("Name of imagery group for ortho-rectification");

    location_opt = G_define_standard_option(G_OPT_M_LOCATION);
    location_opt->key = "target_project";
    location_opt->required = YES;
    location_opt->description =
        _("Name of target project (location) for ortho-rectification");

    mapset_opt = G_define_standard_option(G_OPT_M_MAPSET);
    mapset_opt->key = "target_mapset";
    mapset_opt->required = YES;
    mapset_opt->description =
        _("Name of target mapset for ortho-rectification");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    strcpy(group, group_opt->answer);
    strcpy(target_location, location_opt->answer);
    strcpy(target_mapset, mapset_opt->answer);

    I_get_target(group, location, mapset);
    G_create_alt_env();
    G_setenv_nogisrc("LOCATION_NAME", target_location);
    stat = G_mapset_permissions(target_mapset);
    if (stat != 1) {
        G_fatal_error(_("Unable to access target project/mapset %s/%s"),
                      target_location, target_mapset);
    }

    G_setenv_nogisrc("MAPSET", target_mapset);
    G_get_window(&target_window);
    if (target_window.proj == PROJECTION_XY)
        G_fatal_error(_("Target projects (locations) with XY (unreferenced) "
                        "are not supported"));
    else if (target_window.proj == PROJECTION_LL)
        G_fatal_error(
            _("Target projects (locations) with lon/lat are not supported"));

    G_switch_env();
    I_put_target(group, target_location, target_mapset);

    G_message(_("Group [%s] targeted for project (location) [%s], mapset [%s]"),
              group, target_location, target_mapset);

    exit(EXIT_SUCCESS);
}
