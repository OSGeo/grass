
/***************************************************************
 *
 * MODULE:       v.parallel
 * 
 * AUTHOR(S):    Radim Blazek
 *               
 * PURPOSE:      Create parallel lines
 *               
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 * TODO/BUG:     The vector segments are not properly connected
 *               and should be snapped.
 **************************************************************/
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *in_opt, *out_opt, *distance_opt;
    struct Map_info In, Out;
    struct line_pnts *Points, *Points2;
    struct line_cats *Cats;
    int line, nlines;
    double distance, tolerance;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector, geometry");
    module->description = _("Create parallel line to input lines");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    /* layer_opt = G_define_standard_option(G_OPT_V_FIELD); */

    distance_opt = G_define_option();
    distance_opt->key = "distance";
    distance_opt->type = TYPE_DOUBLE;
    distance_opt->required = YES;
    distance_opt->multiple = NO;
    distance_opt->description =
	_("Offset in map units, positive for right side, "
	  "negative for left side.");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* layer = atoi ( layer_opt->answer ); */
    distance = atof(distance_opt->answer);
    tolerance = distance / 10.;

    Vect_set_open_level(2);
    Vect_open_old(&In, in_opt->answer, "");
    Vect_open_new(&Out, out_opt->answer, 0);
    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    Points = Vect_new_line_struct();
    Points2 = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(&In);

    for (line = 1; line <= nlines; line++) {
	int ltype;

	G_percent(line, nlines, 1);

	ltype = Vect_read_line(&In, Points, Cats, line);

	if (ltype & GV_LINES) {
	    Vect_line_parallel(Points, distance, tolerance, 1, Points2);
	    Vect_write_line(&Out, ltype, Points2, Cats);
	}
	else {
	    Vect_write_line(&Out, ltype, Points, Cats);
	}
    }

    Vect_close(&In);
    Vect_build(&Out);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
