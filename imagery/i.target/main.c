
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
 * PURPOSE:      Targets an imagery group to a GRASS data base project name 
 *               and subproject for reprojection
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
    struct Option *group, *subproject, *loc;
    struct GModule *module;
    struct Flag *c;
    char t_subproject[GMAPSET_MAX], t_project[GMAPSET_MAX];
    char group_name[GNAME_MAX], subproject_name[GMAPSET_MAX];

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("map management"));
    module->description =
	_("Targets an imagery group to a GRASS project and subproject.");

    group = G_define_standard_option(G_OPT_I_GROUP);
    group->gisprompt = "any,group,group";

    loc = G_define_option();
    loc->key = "project";
    loc->type = TYPE_STRING;
    loc->required = NO;
    loc->description = _("Name of imagery target project");

    subproject = G_define_option();
    subproject->key = "subproject";
    subproject->type = TYPE_STRING;
    subproject->required = NO;
    subproject->description = _("Name of target subproject");

    c = G_define_flag();
    c->key = 'c';
    c->description =
	_("Set current project and subproject as target for imagery group");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* check if current subproject:  (imagery libs are very lacking in this dept)
       - abort if not,
       - remove @subproject part if it is
     */
    if (G_name_is_fully_qualified(group->answer, group_name, subproject_name)) {
	if (strcmp(subproject_name, G_subproject()))
	    G_fatal_error(_("Group must exist in the current subproject"));
    }
    else {
	strcpy(group_name, group->answer);	/* FIXME for buffer overflow (have the parser check that?) */
    }

    /* if no setting options are given, print the current target info */
    if (!c->answer && !subproject->answer && !loc->answer) {

	if (I_get_target(group_name, t_project, t_subproject))
	    G_message(_("Group <%s> targeted for project [%s], subproject [%s]"),
		      group_name, t_project, t_subproject);
	else
	    G_message(_("Group <%s> has no target"), group_name);

	exit(EXIT_SUCCESS);
    }

    /* error if -c is specified with other options, or options are incomplete */
    if ((c->answer && (subproject->answer || loc->answer)) ||
	(!c->answer && (!subproject->answer || !loc->answer)))
	G_fatal_error(_("Use either the Current Subproject and "
			"Project Flag (-c)\n OR\n manually enter the variables"));

    if (c->answer) {
	/* point group target to current subproject and project */
	I_put_target(group_name, G_project(), G_subproject());
	G_message(_("Group <%s> targeted for project [%s], subproject [%s]"),
		  group_name, G_project(), G_subproject());
    }
    else {
	/* point group target to specified subproject and project */

	/* TODO: check if it is in current subproject and strip off @subproject part, if present */

	I_put_target(group_name, loc->answer, subproject->answer);
	G_message(_("Group <%s> targeted for project [%s], subproject [%s]"),
		  group_name, loc->answer, subproject->answer);
    }

    G_done_msg(" ");
    exit(EXIT_SUCCESS);
}
