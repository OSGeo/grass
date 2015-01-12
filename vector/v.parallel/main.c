
/***************************************************************
 *
 * MODULE:       v.parallel
 * 
 * AUTHOR(S):    Radim Blazek
 *               Upgraded by Rosen Matev (Google Summer of Code 2008)
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *               
 * PURPOSE:      Create parallel lines
 *               
 * COPYRIGHT:    (C) 2008-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *in_opt, *layer_opt, *out_opt, *dista_opt;
    struct Option *distb_opt, *angle_opt, *side_opt;
    struct Option *tol_opt;
    struct Flag *round_flag, *buf_flag;
    struct Map_info In, Out;
    struct line_pnts *Points, *Points2, *oPoints, **iPoints;
    struct line_cats *Cats;
    
    char *desc;

    int line, nlines, inner_count, j;
    int layer;
    double da, db, dalpha, tolerance;
    int side;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("buffer"));
    G_add_keyword(_("line"));
    module->description = _("Creates parallel line to input vector lines.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    
    layer_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    
    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);


    dista_opt = G_define_option();
    dista_opt->key = "distance";
    dista_opt->type = TYPE_DOUBLE;
    dista_opt->required = YES;
    dista_opt->options = "0-100000000";
    dista_opt->multiple = NO;
    dista_opt->description = _("Offset along major axis in map units");

    distb_opt = G_define_option();
    distb_opt->key = "minordistance";
    distb_opt->type = TYPE_DOUBLE;
    distb_opt->required = NO;
    distb_opt->options = "0-100000000";
    distb_opt->multiple = NO;
    distb_opt->description = _("Offset along minor axis in map units");

    angle_opt = G_define_option();
    angle_opt->key = "angle";
    angle_opt->type = TYPE_DOUBLE;
    angle_opt->required = NO;
    angle_opt->answer = "0";
    angle_opt->multiple = NO;
    angle_opt->description = _("Angle of major axis in degrees"); /* CW or CCW? does this even work?? */

    side_opt = G_define_option();
    side_opt->key = "side";
    side_opt->type = TYPE_STRING;
    side_opt->required = YES;
    side_opt->answer = "right";
    side_opt->multiple = NO;
    side_opt->options = "left,right,both";
    side_opt->description = _("Side");
    desc = NULL;
    G_asprintf(&desc,
	       "left;%s;right;%s;both;%s",
	       _("Parallel line is on the left"),
	       _("Parallel line is on the right"),
	       _("Parallel lines on both sides"));
    side_opt->descriptions = desc;

    tol_opt = G_define_option();
    tol_opt->key = "tolerance";
    tol_opt->type = TYPE_DOUBLE;
    tol_opt->required = NO;
    tol_opt->options = "0-100000000";
    tol_opt->multiple = NO;
    tol_opt->description = _("Tolerance of arc polylines in map units");

    round_flag = G_define_flag();
    round_flag->key = 'r';
    round_flag->description = _("Make outside corners round");

    buf_flag = G_define_flag();
    buf_flag->key = 'b';
    buf_flag->description = _("Create buffer-like parallel lines");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    da = atof(dista_opt->answer);

    if (distb_opt->answer)
	db = atof(distb_opt->answer);
    else
	db = da;

    if (angle_opt->answer)
	dalpha = atof(angle_opt->answer);
    else
	dalpha = 0;

    if (tol_opt->answer)
	tolerance = atof(tol_opt->answer);
    else
	tolerance = ((db < da) ? db : da) / 100.;

    if (strcmp(side_opt->answer, "right") == 0)
	side = 1;
    else if (strcmp(side_opt->answer, "left") == 0)
	side = -1;
    else
	side = 0;

    Vect_set_open_level(2);

    if (Vect_open_old2(&In, in_opt->answer, "", layer_opt->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), in_opt->answer);

    layer = Vect_get_field_number(&In, layer_opt->answer);

    if (Vect_open_new(&Out, out_opt->answer, 0) < 0)
	G_fatal_error(_("Unable to create vector map <%s>"), out_opt->answer);
    
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

	if (layer != -1 && !Vect_cat_get(Cats, layer, NULL))
	    continue;
	
	if (ltype & GV_LINES) {
	    if (!(buf_flag->answer)) {
		if (side != 0) {
		    Vect_line_parallel2(Points, da, db, dalpha, side,
					round_flag->answer, tolerance,
					Points2);
		    Vect_write_line(&Out, ltype, Points2, Cats);
		}
		else {
		    Vect_line_parallel2(Points, da, db, dalpha, 1,
					round_flag->answer, tolerance,
					Points2);
		    Vect_write_line(&Out, ltype, Points2, Cats);
		    Vect_line_parallel2(Points, da, db, dalpha, -1,
					round_flag->answer, tolerance,
					Points2);
		    Vect_write_line(&Out, ltype, Points2, Cats);
		}
	    }
	    else {
		Vect_line_buffer2(Points, da, db, dalpha, round_flag->answer,
				  1, tolerance, &oPoints, &iPoints,
				  &inner_count);
		Vect_write_line(&Out, ltype, oPoints, Cats);
		for (j = 0; j < inner_count; j++) {
		    Vect_write_line(&Out, ltype, iPoints[j], Cats);
		}
	    }
	}
	else {
	    Vect_write_line(&Out, ltype, Points, Cats);
	}
    }

    /*    Vect_build_partial(&Out, GV_BUILD_BASE);
       Vect_snap_lines(&Out, GV_BOUNDARY, 1e-7, NULL);
       Vect_break_lines(&Out, GV_BOUNDARY, NULL);
       Vect_remove_duplicates(&Out, GV_BOUNDARY, NULL);
       Vect_build_partial (&Out, GV_BUILD_NONE);
     */
    Vect_build(&Out);

    Vect_close(&In);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
