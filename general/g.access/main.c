
/****************************************************************************
 *
 * MODULE:       g.access
 * AUTHOR(S):    Michael Shapiro CERL (original contributor)
 *               Markus Neteler <neteler itc.it>
 *               Bernhard Reiter <bernhard intevation.de>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>, Radim Blazek <radim.blazek gmail.com>
 * PURPOSE:      Controls access to the current mapset for other users on the system
 * COPYRIGHT:    (C) 1999-2006, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

int main(int argc, char *argv[])
{
    char path[GPATH_MAX];
    int perms;			/* full mapset permissions */
    int group, other;		/* bool. want group/other permission */
    struct Option *group_opt, *other_opt;
    struct GModule *module;

    /* init the GRASS library */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("permission"));
    module->label =
	_("Controls access to the current mapset for other users on the system.");
    module->description = _("If no option given, prints current status.");
    
    group_opt = G_define_option();
    group_opt->key = "group";
    group_opt->type = TYPE_STRING;
    group_opt->required = NO;
    group_opt->options = "grant,revoke";
    group_opt->description = _("Access for group");
    group_opt->guisection = _("Settings");

    other_opt = G_define_option();
    other_opt->key = "other";
    other_opt->type = TYPE_STRING;
    other_opt->required = NO;
    other_opt->options = "grant,revoke";
    other_opt->description = _("Access for others");
    other_opt->guisection = _("Settings");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

#ifdef __MINGW32__
    G_fatal_error(_("UNIX filesystem access controls are not supported by MS-Windows"));
#endif

    /* get the unix file name for the mapset directory */
    G_file_name(path, "", "", G_mapset());

    /* this part is until PERMANENT no longer holds DEFAULT_WIND and MYNAME */
    if (strcmp(G_mapset(), "PERMANENT") == 0)
	G_fatal_error(_("Access to the PERMANENT mapset must be open, nothing changed"));

    /* get the current permissions */
    if (get_perms(path, &perms, &group, &other) < 0)
	G_fatal_error(_("Unable to determine mapset permissions"));

    if (group_opt->answer) {
	if (group_opt->answer[0] == 'g')
	    group = 1;
	else
	    group = 0;
    }
    if (other_opt->answer) {
	if (other_opt->answer[0] == 'g')
	    other = 1;
	else
	    other = 0;
    }

    set_perms(path, perms, group, other);

    exit(EXIT_SUCCESS);
}
