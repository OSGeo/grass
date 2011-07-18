/*
 ****************************************************************************
 *
 * MODULE:       d.info
 * AUTHOR(S):    Glynn Clements
 * PURPOSE:      Display information about the active display monitor
 * COPYRIGHT:    (C) 2004 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Flag *rflag, *dflag, *fflag, *bflag, *gflag;
    double t, b, l, r;
    double n, s, e, w;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("metadata"));
    module->description =
	_("Display information about the active display monitor");

    rflag = G_define_flag();
    rflag->key = 'r';
    rflag->description =
	_("Display screen rectangle (left, right, top, bottom)");

    dflag = G_define_flag();
    dflag->key = 'd';
    dflag->description = _("Display screen dimensions (width, height)");

    fflag = G_define_flag();
    fflag->key = 'f';
    fflag->description = _("Display active frame rectangle");

    bflag = G_define_flag();
    bflag->key = 'b';
    bflag->description = _("Display screen rectangle of current region");

    gflag = G_define_flag();
    gflag->key = 'g';
    gflag->description =
	_("Display geographic coordinates and resolution of entire screen");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (!rflag->answer && !dflag->answer &&
	!fflag->answer && !bflag->answer && !gflag->answer) {
	G_usage();
	exit(EXIT_FAILURE);
    }

    if (D_open_driver() != 0)
	G_fatal_error(_("No graphics device selected. "
			"Use d.mon to select graphics device."));
    
    if (rflag->answer || dflag->answer || fflag->answer)
	D_get_window(&t, &b, &l, &r);


    if (rflag->answer)
	fprintf(stdout, "rectangle: %f %f %f %f\n", l, r, t, b);

    if (dflag->answer)
	fprintf(stdout, "dimensions: %f %f\n", r - l, b - t);

    if (fflag->answer)
	fprintf(stdout, "frame: %f %f %f %f\n", l, r, t, b);

    if (bflag->answer) {
	D_setup(0);

	l = D_get_d_west();
	r = D_get_d_east();
	t = D_get_d_north();
	b = D_get_d_south();

	fprintf(stdout, "region: %f %f %f %f\n", l, r, t, b);
    }

    if (gflag->answer) {
	/* outer bounds of the screen (including margins) */
	D_setup(0);

	n = D_get_u_north();
	s = D_get_u_south();
	w = D_get_u_west();
	e = D_get_u_east();

	fprintf(stdout, "n=%f\n", n );
	fprintf(stdout, "s=%f\n", s );
	fprintf(stdout, "w=%f\n", w );
	fprintf(stdout, "e=%f\n", e );
	fprintf(stdout, "ewres=%.15g\n", (e-w)/(r-l) );
	fprintf(stdout, "nsres=%.15g\n", (n-s)/(b-t) );
    }

    
    D_close_driver();

    return EXIT_SUCCESS;
}
