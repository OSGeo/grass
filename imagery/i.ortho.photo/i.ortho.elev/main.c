
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
    struct Option *group_opt, *loc_opt, *mapset_opt, *elev_opt;
    struct Option *math_opt, *unit_opt, *nd_opt;
    struct Flag *list_flag, *print_flag;

    char location[GMAPSET_MAX];
    char mapset[GMAPSET_MAX];
    char group[GNAME_MAX];

    char *elev_layer;
    char *mapset_elev;
    char *location_elev;
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

    loc_opt = G_define_standard_option(G_OPT_M_LOCATION);
    loc_opt->required = NO;
    loc_opt->description =
	_("Name of the target location");

    mapset_opt = G_define_standard_option(G_OPT_M_MAPSET);
    mapset_opt->required = NO;
    mapset_opt->description =
	_("Name of the target mapset");

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
    mapset_elev = (char *)G_malloc(GMAPSET_MAX * sizeof(char));
    location_elev = (char *)G_malloc(80 * sizeof(char));
    math_exp = (char *)G_malloc(80 * sizeof(char));
    units = (char *)G_malloc(80 * sizeof(char));
    nd = (char *)G_malloc(80 * sizeof(char));

    *elev_layer = 0;
    *mapset_elev = 0;
    *location_elev = 0;
    *math_exp = 0;
    *units = 0;
    *nd = 0;

    strcpy(group, group_opt->answer);
    if(loc_opt->answer)
    	strcpy(location_elev, loc_opt->answer);
    if(mapset_opt->answer)
    	strcpy(mapset_elev, mapset_opt->answer);
    /*if(elev_opt->answer)
    	strcpy(elev_layer, elev_opt->answer);*/
    if(math_opt->answer)
    	strcpy(math_exp, math_opt->answer);
    if(unit_opt->answer)
    	strcpy(units, unit_opt->answer);
    if(nd_opt->answer)
    	strcpy(nd, nd_opt->answer);
	
    if (!I_get_target(group, location, mapset)) {
	G_fatal_error(_("Please select a target for group [%s] first"), group);
    }

    sprintf(buf, "%s/%s", G_gisdbase(), location);
    if (access(buf, 0) != 0) {
	G_fatal_error(_("Target location [%s] not found\n"), location);
    }

    /*Report the contents of the ELEVATION file as in the GROUP */
    if (print_flag->answer) {
	/*If the content is empty report an error */
	if(!I_get_group_elev(group, elev_layer, mapset_elev, location_elev, math_exp, units, nd)){
		G_fatal_error(_("Cannot find default elevation map for target in group [%s]"),group);
	/*If there is a content, report it as a message */
	} else {
		G_message("map:\t\t\t%s",elev_layer);
		G_message("mapset:\t\t\t%s",mapset_elev);
		G_message("location:\t\t%s",location_elev);
		G_message("math expression:\t%s",math_exp);
		G_message("units:\t\t\t%s",units);
		G_message("nodata value:\t\t%s",nd);
		exit(EXIT_SUCCESS);
	}    
    }

    /*Creating a Target environment*/
    G_create_alt_env();
    /*Set an alternate Location */
    G_setenv_nogisrc("LOCATION_NAME", location);
    /*Check for permissions in mapset */
    stat = G_mapset_permissions(mapset);
    /*We have permissions on the mapset */
    if (stat > 0) {
	/*Set the alternate mapset */
	G_setenv_nogisrc("MAPSET", mapset);
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
	/*Start working on the Target mapset */
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
	
	/* return to current Location/mapset to write in the group file */
	select_current_env();
	
	/* load information from the ELEVATION file in the GROUP */
	I_get_group_elev(group, elev_layer, mapset_elev, location_elev, math_exp, units, nd);
	/* Modify ELEVATION file in source GROUP */
	I_put_group_elev(group,elev_opt->answer,mapset_opt->answer,loc_opt->answer, 
			math_opt->answer, unit_opt->answer, nd_opt->answer);
	/* select current location */
	select_current_env();

        G_message(_("Group [%s] in location [%s] mapset [%s] now uses elevation map [%s]"),
	          group, location, mapset, elev_opt->answer);
    }else{
	G_fatal_error(_("Mapset [%s] in target location [%s] - %s "),
                  mapset, location,
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
