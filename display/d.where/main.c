
/****************************************************************************
 *
 * MODULE:       d.where
 * AUTHOR(S):    James Westervelt and Michael Shapiro (CERL) 
                  (original contributors)
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Markus Neteler <neteler itc.it>,
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>, 
 *               Paul Kelly <paul-grass stjohnspoint.co.uk>
 * PURPOSE:      interactive query of location in active display
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/display.h>
#include "local_proto.h"
#include <grass/glocale.h>

struct pj_info iproj, oproj, tproj;

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *coords, *file;
    struct Flag *decimal, *latlong, *wgs84, *dcoord;
    int have_spheroid = 0;
    FILE *fp;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("sampling"));
    G_add_keyword(_("position"));
    G_add_keyword(_("querying"));
    module->description =
	_("Identifies the geographic coordinates associated with "
	  "point locations given in display coordinates.");

    coords = G_define_option();
    coords->key = "at";
    coords->key_desc = "x,y";
    coords->type = TYPE_DOUBLE;
    coords->required = NO;
    coords->multiple = YES;
    coords->description =
	_("Display coordinates to convert");

    file = G_define_standard_option(G_OPT_F_INPUT);
    file->required = NO;
    file->description =
	_("File from which to read coordinates (\"-\" to read from stdin)");

    decimal = G_define_flag();
    decimal->key = 'd';
    decimal->description = _("Output lat/long in decimal degree");

    latlong = G_define_flag();
    latlong->key = 'l';
    latlong->description =
	_("Output lat/long referenced to current ellipsoid");

    wgs84 = G_define_flag();
    wgs84->key = 'w';
    wgs84->description =
	_("Output lat/long referenced to WGS84 ellipsoid using datum "
	  "transformation parameters defined in current location (if available)");

    dcoord = G_define_flag();
    dcoord->key = 'f';
    dcoord->description =
	_("Output frame coordinates of current display monitor (percentage)");


    /* if (G_parser(argc,argv)) */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (latlong->answer && wgs84->answer)
	G_fatal_error(_("Ambiguous request for lat/long ellipsoids"));

    if (decimal->answer && !(latlong->answer || wgs84->answer))
	G_fatal_error(_("Please specify a lat/long ellipsoid with -l or -w"));

    if (((G_projection() == PROJECTION_LL) && wgs84->answer) ||
	((G_projection() != PROJECTION_LL) &&
	 (latlong->answer || wgs84->answer)))
	have_spheroid = 1;

    if (have_spheroid == 1) {
	struct Key_Value *in_proj_info, *in_unit_info;
	char buff[100], dum[100];

	/* read current projection info */
	if ((in_proj_info = G_get_projinfo()) == NULL)
	    G_fatal_error(_("Can't get projection info of current location"));

	if ((in_unit_info = G_get_projunits()) == NULL)
	    G_fatal_error(_("Can't get projection units of current location"));

	if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
	    G_fatal_error(_("Can't get projection key values of current location"));

	oproj.pj = NULL;

	if (wgs84->answer) {
	    struct Key_Value *out_proj_info, *out_unit_info;

	    out_proj_info = G_create_key_value();
	    out_unit_info = G_create_key_value();

	    /* set output projection to lat/long */
	    G_set_key_value("proj", "ll", out_proj_info);

	    /* Check that datumparams are defined for this location (otherwise
	     * the WGS84 values would be meaningless), and if they are set the 
	     * output datum to WGS84 */
#if PROJ_VERSION_MAJOR < 6
	    /* PROJ6+ has its own datum transformation parameters */
	    if (G_get_datumparams_from_projinfo(in_proj_info, buff, dum) < 0)
		G_fatal_error(_("WGS84 output not possible as this location does not contain\n"
			       "datum transformation parameters. Try running g.setproj."));
	    else
#endif
		G_set_key_value("datum", "wgs84", out_proj_info);

	    G_set_key_value("unit", "degree", out_unit_info);
	    G_set_key_value("units", "degrees", out_unit_info);
	    G_set_key_value("meters", "1.0", out_unit_info);

	    if (pj_get_kv(&oproj, out_proj_info, out_unit_info) < 0)
		G_fatal_error(_("Unable to set up lat/long projection parameters"));

	    G_free_key_value(out_proj_info);
	    G_free_key_value(out_unit_info);
	}
	/* else the latlong equivalent is generated by GPJ_init_transform() */

	G_free_key_value(in_proj_info);
	G_free_key_value(in_unit_info);

	tproj.def = NULL;

	if (GPJ_init_transform(&iproj, &oproj, &tproj) < 0)
	    G_fatal_error(_("Unable to initialize coordinate transformation"));
    }

    if (file->answer) {
	if (strcmp(file->answer, "-") == 0)
	    fp = stdin;
	else {
	    fp = fopen(file->answer, "r");
	    if (!fp)
		G_fatal_error(_("Unable to open input file <%s>"), file->answer);
	}
    }
    else
	fp = stdin;

    D_open_driver();

    D_setup(0);

    where_am_i(coords->answers, fp, have_spheroid, decimal->answer,
               dcoord->answer);
    
    D_close_driver();

    exit(EXIT_SUCCESS);
}
