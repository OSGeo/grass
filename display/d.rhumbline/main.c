
/****************************************************************************
 *
 * MODULE:       d.rhumbline
 * AUTHOR(S):    Michael Shapiro (CERL) (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      displays the rhumbline joining two user-specified points
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/* TODO: implement G_rhumbline_distance() in libgis 
 */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "local_proto.h"

int main(int argc, char *argv[])
{
    int line_color;
    int text_color = 0;
    double lon1, lat1, lon2, lat2;
    struct GModule *module;
    struct
    {
	struct Option *lcolor, *tcolor, *coor;
    } parm;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("distance"));
    module->description =
	_("Displays the rhumbline joining two longitude/latitude coordinates.");

    parm.coor = G_define_option();
    parm.coor->key = "coor";
    parm.coor->key_desc = "lon1,lat1,lon2,lat2";
    parm.coor->type = TYPE_STRING;
    parm.coor->required = YES;
    parm.coor->description = _("Starting and ending coordinates");

    parm.lcolor = G_define_option();
    parm.lcolor->key = "lcolor";
    parm.lcolor->type = TYPE_STRING;
    parm.lcolor->required = NO;
    parm.lcolor->description = _("Line color");
    parm.lcolor->gisprompt = "old_color,color,color";
    parm.lcolor->answer = DEFAULT_FG_COLOR;

#ifdef CAN_DO_DISTANCES
    parm.tcolor = G_define_option();
    parm.tcolor->key = "tcolor";
    parm.tcolor->type = TYPE_STRING;
    parm.tcolor->required = NO;
    parm.tcolor->description = _("Text color");
    parm.tcolor->gisprompt = "old_color,color,color";
#endif

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (G_projection() != PROJECTION_LL)
	G_fatal_error(_("Location is not %s"),
		      G__projection_name(PROJECTION_LL));

    if (parm.coor->answers[0] == NULL)
	G_fatal_error(_("No coordinates given"));

    if (!G_scan_easting(parm.coor->answers[0], &lon1, G_projection()))
	G_fatal_error(_("%s - illegal longitude"), parm.coor->answers[0]);

    if (!G_scan_northing(parm.coor->answers[1], &lat1, G_projection()))
	G_fatal_error(_("%s - illegal longitude"), parm.coor->answers[1]);

    if (!G_scan_easting(parm.coor->answers[2], &lon2, G_projection()))
	G_fatal_error(_("%s - illegal longitude"), parm.coor->answers[2]);

    if (!G_scan_northing(parm.coor->answers[3], &lat2, G_projection()))
	G_fatal_error(_("%s - illegal longitude"), parm.coor->answers[3]);


    if (D_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));

    line_color = D_translate_color(parm.lcolor->answer);
    if (!line_color)
	line_color = D_translate_color(parm.lcolor->answer =
				       DEFAULT_FG_COLOR);

#ifdef CAN_DO_DISTANCES
    if (strcmp(parm.lcolor->answer, DEFAULT_FG_COLOR) == 0)
	deftcolor = "red";
    else
	deftcolor = DEFAULT_FG_COLOR;

    if (parm.tcolor->answer == NULL)
	parm.tcolor->answer = deftcolor;
    text_color = D_translate_color(parm.tcolor->answer);
    if (!text_color)
	text_color = D_translate_color(deftcolor);
#endif

    plot(lon1, lat1, lon2, lat2, line_color, text_color);

    D_close_driver();

    exit(EXIT_SUCCESS);
}
