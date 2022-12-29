
/****************************************************************************
 *
 * MODULE:       d.geodesic
 * AUTHOR(S):    Michael Shapiro (CERL) (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      displays a geodesic line in the active frame on the user's 
 *               graphics monitor
 * COPYRIGHT:    (C) 1999-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "local_proto.h"

int main(int argc, char *argv[])
{
    int line_color;
    int text_color;
    double lon1, lat1, lon2, lat2;
    char *deftcolor;
    const char *unit;
    int unit_id;
    double factor;
    struct GModule *module;
    struct
    {
	struct Option *lcolor, *tcolor, *coor, *units;
    } parm;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("distance"));
    G_add_keyword(_("great circle"));
    G_add_keyword(_("shortest path"));
    module->description =
	_("Displays a geodesic line, tracing the shortest distance "
	"between two geographic points along a great circle, in "
	"a longitude/latitude data set.");

    parm.coor = G_define_standard_option(G_OPT_M_COORDS);
    parm.coor->key_desc = "lon1,lat1,lon2,lat2";
    parm.coor->required = YES;
    parm.coor->description = _("Starting and ending coordinates");

    parm.lcolor = G_define_standard_option(G_OPT_C);
    parm.lcolor->key = "line_color";
    parm.lcolor->label = _("Line color");

    parm.tcolor = G_define_standard_option(G_OPT_C);
    parm.tcolor->key = "text_color";
    parm.tcolor->label = _("Text color");
    parm.tcolor->answer = NULL;

    parm.units = G_define_standard_option(G_OPT_M_UNITS);
    parm.units->options = "meters,kilometers,feet,miles";
    parm.units->label = parm.units->description;
    parm.units->answer = "meters";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (G_projection() != PROJECTION_LL)
	G_fatal_error(_("Location is not %s"),
		      G_projection_name(PROJECTION_LL));

    /* get conversion factor and unit name */
    unit_id = G_units(parm.units->answer);
    factor = 1. / G_meters_to_units_factor(unit_id);
    unit = G_get_units_name(unit_id, 1, 0);

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


    D_open_driver();
    
    line_color = D_translate_color(parm.lcolor->answer);
    if (!line_color)
	line_color = D_translate_color(parm.lcolor->answer =
				       DEFAULT_FG_COLOR);

    if (strcmp(parm.lcolor->answer, DEFAULT_FG_COLOR) == 0)
	deftcolor = "red";
    else
	deftcolor = DEFAULT_FG_COLOR;

    if (parm.tcolor->answer == NULL)
	text_color = D_translate_color(deftcolor);
    else if (strcmp(parm.tcolor->answer, "none") == 0)
	text_color = -1;
    else
	text_color = D_translate_color(parm.tcolor->answer);

    plot(lon1, lat1, lon2, lat2, line_color, text_color, factor, unit);

    D_save_command(G_recreate_command());
    D_close_driver();

    exit(EXIT_SUCCESS);
}
