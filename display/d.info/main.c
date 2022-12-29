/*
 ****************************************************************************
 *
 * MODULE:       d.info
 * AUTHOR(S):    Glynn Clements
 * PURPOSE:      Display information about the active display monitor
 * COPYRIGHT:    (C) 2004-2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Flag *rflag, *dflag, *fflag, *eflag, *bflag, *gflag;
    double st, sb, sl, sr;
    double ft, fb, fl, fr;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("graphics"));
    G_add_keyword(_("monitors"));
    module->label =
	_("Displays information about the active display monitor.");
    module->description =
	_("Display monitors are maintained by d.mon.");

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

    eflag = G_define_flag();
    eflag->key = 'e';
    eflag->description = _("Display frame dimensions (width, height)");

    bflag = G_define_flag();
    bflag->key = 'b';
    bflag->description = _("Display screen rectangle of current region");

    gflag = G_define_flag();
    gflag->key = 'g';
    gflag->description =
	_("Display geographic coordinates and resolution of entire frame");

    G_option_required(rflag, dflag, fflag, eflag, bflag, gflag, NULL);
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    D_open_driver();
    
    if (rflag->answer || dflag->answer)
	D_get_screen(&st, &sb, &sl, &sr);
    
    if (fflag->answer || eflag->answer || gflag->answer)
	D_get_frame(&ft, &fb, &fl, &fr);


    if (rflag->answer)
	fprintf(stdout, "screen rectangle: %f %f %f %f\n", sl, sr, st, sb);

    if (dflag->answer)
	fprintf(stdout, "screen dimensions: %f %f\n", sr - sl, sb - st);

    if (fflag->answer)
	fprintf(stdout, "frame rectangle: %f %f %f %f\n", fl, fr, ft, fb);

    if (eflag->answer)
	fprintf(stdout, "frame dimensions: %f %f\n", fr - fl, fb - ft);

    if (bflag->answer) {
	double t, b, l, r;
	D_setup(0);

	l = D_get_d_west();
	r = D_get_d_east();
	t = D_get_d_north();
	b = D_get_d_south();

	fprintf(stdout, "region: %f %f %f %f\n", l, r, t, b);
    }

    if (gflag->answer) {
	/* outer bounds of the screen (including margins) */
	double n, s, e, w;
	D_setup(0);

	n = D_d_to_u_row(ft);
	s = D_d_to_u_row(fb);
	w = D_d_to_u_col(fl);
	e = D_d_to_u_col(fr);

	fprintf(stdout, "n=%f\n", n );
	fprintf(stdout, "s=%f\n", s );
	fprintf(stdout, "w=%f\n", w );
	fprintf(stdout, "e=%f\n", e );
	fprintf(stdout, "ewres=%.15g\n",  D_get_d_to_u_xconv() );
	fprintf(stdout, "nsres=%.15g\n", -D_get_d_to_u_yconv() );
    }

   
    D_close_driver();

    exit(EXIT_SUCCESS);
}
