
/****************************************************************************
 *
 * MODULE:       i.target
 * AUTHOR(S):    Michael Shapiro (USACERL) and Bob Covill (original contributors)
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>
 *
 *               Original INTER author: M. Shapiro
 *               New cmd line version by Bob Covill 10/2001
 *               Rewritten for GRASS6 by Brad Douglas 08/2005
 *               Output existing group into by Hamish Bowman 6/2007
 *
 * PURPOSE:      Targets an imagery group to a GRASS data base location name 
 *               and mapset for reprojection
 * COPYRIGHT:    (C) 2001-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>


int main(int argc, char *argv[])
{
    struct Option *group, *mapset, *loc;
    struct GModule *module;
    struct Flag *c;
    char t_mapset[GMAPSET_MAX], t_location[GMAPSET_MAX];
    char group_name[GNAME_MAX], mapset_name[GMAPSET_MAX];

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("map management"));
    module->description =
	_("Targets an imagery group to a GRASS location and mapset.");

    group = G_define_standard_option(G_OPT_I_GROUP);
    group->gisprompt = "any,group,group";

    loc = G_define_option();
    loc->key = "location";
    loc->type = TYPE_STRING;
    loc->required = NO;
    loc->description = _("Name of imagery target location");

    mapset = G_define_option();
    mapset->key = "mapset";
    mapset->type = TYPE_STRING;
    mapset->required = NO;
    mapset->description = _("Name of target mapset");

    c = G_define_flag();
    c->key = 'c';
    c->description =
	_("Set current location and mapset as target for imagery group");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* check if current mapset:  (imagery libs are very lacking in this dept)
       - abort if not,
       - remove @mapset part if it is
     */
    if (G_name_is_fully_qualified(group->answer, group_name, mapset_name)) {
	if (strcmp(mapset_name, G_mapset()))
	    G_fatal_error(_("Group must exist in the current mapset"));
    }
    else {
	strcpy(group_name, group->answer);	/* FIXME for buffer overflow (have the parser check that?) */
    }

    /* if no setting options are given, print the current target info */
    if (!c->answer && !mapset->answer && !loc->answer) {

	if (I_get_target(group_name, t_location, t_mapset))
	    G_message(_("Group <%s> targeted for location [%s], mapset [%s]"),
		      group_name, t_location, t_mapset);
	else
	    G_message(_("Group <%s> has no target"), group_name);

	exit(EXIT_SUCCESS);
    }

    /* error if -c is specified with other options, or options are incomplete */
    if ((c->answer && (mapset->answer || loc->answer)) ||
	(!c->answer && (!mapset->answer || !loc->answer)))
	G_fatal_error(_("Use either the Current Mapset and "
			"Location Flag (-c)\n OR\n manually enter the variables"));

    if (c->answer) {
	/* point group target to current mapset and location */
	I_put_target(group_name, G_location(), G_mapset());
	G_message(_("Group <%s> targeted for location [%s], mapset [%s]"),
		  group_name, G_location(), G_mapset());
    }
    else {
	/* point group target to specified mapset and location */

	/* TODO: check if it is in current mapset and strip off @mapset part, if present */

	I_put_target(group_name, loc->answer, mapset->answer);
	G_message(_("Group <%s> targeted for location [%s], mapset [%s]"),
		  group_name, loc->answer, mapset->answer);
    }

    G_done_msg(" ");
    exit(EXIT_SUCCESS);
}
