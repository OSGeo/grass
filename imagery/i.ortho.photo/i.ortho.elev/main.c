
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
    struct Option *group_opt, *elev_opt;
    struct Flag *list_flag, *print_flag;

    char location[GMAPSET_MAX];
    char mapset[GMAPSET_MAX];
    char group[GNAME_MAX];

    char *elev_layer;
    char *mapset_elev;
    char *tl;
    char *math_exp;
    char *units;
    char *nd;

    char buf[100];
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

    elev_opt = G_define_standard_option(G_OPT_R_ELEV);
    elev_opt->required = NO;
    elev_opt->description =
	_("Name of elevation map to use for ortho-rectification");

    list_flag = G_define_flag();
    list_flag->key = 'l';
    list_flag->description =
	_("List available raster maps in target mapset and exit");

    print_flag = G_define_flag();
    print_flag->key = 'p';
    print_flag->description =
	_("Print currently selected elevation map and exit");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    elev_layer = (char *)G_malloc(GNAME_MAX * sizeof(char));
    mapset_elev = (char *)G_malloc(GMAPSET_MAX * sizeof(char));
    tl = (char *)G_malloc(80 * sizeof(char));
    math_exp = (char *)G_malloc(80 * sizeof(char));
    units = (char *)G_malloc(80 * sizeof(char));
    nd = (char *)G_malloc(80 * sizeof(char));

    *elev_layer = 0;
    *mapset_elev = 0;
    *tl = 0;
    *math_exp = 0;
    *units = 0;
    *nd = 0;

    strcpy(group, group_opt->answer);

    G_suppress_warnings(1);
    if (!I_get_target(group, location, mapset)) {
	G_fatal_error(_("Please select a target for group [%s] first"), group);
    }

    G_suppress_warnings(0);
    sprintf(buf, "%s/%s", G_gisdbase(), location);
    if (access(buf, 0) != 0) {
	G_fatal_error(_("Target location [%s] not found\n"), location);
    }

    if (print_flag->answer) {
	I_get_group_elev(group, elev_layer, mapset_elev, tl, math_exp, units, nd);
	
	exit(EXIT_SUCCESS);
    }


    G__create_alt_env();
    G__setenv("LOCATION_NAME", location);

    stat = G__mapset_permissions(mapset);
    if (stat > 0) {
	G__setenv("MAPSET", mapset);
	G__create_alt_search_path();
	G__switch_env();
	G__switch_search_path();
	which_env = 0;

	select_target_env();
	
	if (list_flag->answer) {
	    /* list raster maps in target location */

	    /* select current location */
	    select_current_env();
	}
	else {
	    if (!elev_opt->answer) {
		/* select current location */
		select_current_env();
		G_fatal_error(_("Elevation map name is missing. Please set '%s' option"),
		    elev_opt->key);
	    }
	    /* elevation map exists in target ? */
	    
	    if (G_find_raster2(elev_opt->answer, mapset) == NULL) {
		/* select current location */
		select_current_env();
		G_fatal_error(_("Raster map <%s> not found"), elev_opt->answer);
	    }

	    I_get_group_elev(group, elev_layer, mapset_elev, tl, math_exp, units, nd);
	    overwrite = G__getenv("OVERWRITE") != NULL;
	    
	    if (!overwrite && *elev_layer) {
		G_warning(_("Elevation for group <%s> is already set to <%s>"),
		          group, elev_layer);
	    }

	    I_put_group_elev(group, elev_opt->answer, mapset_elev, tl,
			     math_exp, units, nd);
	    /* select current location */
	    select_current_env();
	}

	exit(EXIT_SUCCESS);
    }

    G_fatal_error(_("Mapset [%s] in target location [%s] - %s "),
                  mapset, location,
		  stat == 0 ? _("permission denied\n") : _("not found\n"));
}


int select_current_env(void)
{
    if (which_env != 0) {
	G__switch_env();
	G__switch_search_path();
	which_env = 0;
    }

    return 0;
}

int select_target_env(void)
{
    if (which_env != 1) {
	G__switch_env();
	G__switch_search_path();
	which_env = 1;
    }

    return 0;
}
