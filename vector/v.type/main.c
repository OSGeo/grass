/***************************************************************
 *
 * MODULE:       v.type
 * 
 * AUTHOR(S):    Radim Blazek
 *               OGR support by Martin Landa <landa.martin gmail.com>               
 *
 * PURPOSE:      Feature type manipulations
 *               
 * COPYRIGHT:    (C) 2001-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    static struct line_pnts *Points;
    struct line_cats *Cats;
    int type, field;
    struct GModule *module;

    struct Option *in_opt, *out_opt, *to_opt, *from_opt, *field_opt;
    int from_type, to_type;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("editing"));
    G_add_keyword(_("area"));
    G_add_keyword(_("line"));
    G_add_keyword(_("point"));
    module->description = _("Changes type of vector features.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    
    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    from_opt = G_define_standard_option(G_OPT_V_TYPE);
    from_opt->key = "from_type";
    from_opt->options = "point,line,boundary,centroid,face,kernel";
    from_opt->required = YES;
    from_opt->multiple = NO;
    from_opt->description = _("Feature type to convert from");
    from_opt->answer = "line";

    to_opt = G_define_standard_option(G_OPT_V_TYPE);
    to_opt->key = "to_type";
    to_opt->options = "point,line,boundary,centroid,face,kernel";
    to_opt->required = YES;
    to_opt->multiple = NO;
    to_opt->description = _("Feature type to convert to");
    to_opt->answer = "boundary";
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Vect_check_input_output_name(in_opt->answer, out_opt->answer, G_FATAL_EXIT);
			    
    switch (from_opt->answer[0]) {
    case 'p':
	from_type = GV_POINT;
	break;
    case 'l':
	from_type = GV_LINE;
	break;
    case 'b':
	from_type = GV_BOUNDARY;
	break;
    case 'c':
	from_type = GV_CENTROID;
	break;
    case 'f':
	from_type = GV_FACE;
	break;
    case 'k':
	from_type = GV_KERNEL;
	break;
    }
    switch (to_opt->answer[0]) {
    case 'p':
	to_type = GV_POINT;
	break;
    case 'l':
	to_type = GV_LINE;
	break;
    case 'b':
	to_type = GV_BOUNDARY;
	break;
    case 'c':
	to_type = GV_CENTROID;
	break;
    case 'f':
	to_type = GV_FACE;
	break;
    case 'k':
	to_type = GV_KERNEL;
	break;
    }
    /* check type compatibility */
    if (((from_type & (GV_POINT | GV_CENTROID | GV_KERNEL)) &&
	 (to_type & (GV_LINE | GV_BOUNDARY | GV_FACE))
	) || ((from_type & (GV_LINE | GV_BOUNDARY | GV_FACE)) &&
	      (to_type & (GV_POINT | GV_CENTROID | GV_KERNEL))
	)
	)
	G_fatal_error(_("Incompatible types"));

    Vect_check_input_output_name(in_opt->answer, out_opt->answer,
				 G_FATAL_EXIT);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* open input vector */
    Vect_set_open_level(1);
    if (Vect_open_old2(&In, in_opt->answer, "", field_opt->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), in_opt->answer);

    field = Vect_get_field_number(&In, field_opt->answer);
    
    if (0 > Vect_open_new(&Out, out_opt->answer, Vect_is_3d(&In))) {
	Vect_close(&In);
	exit(EXIT_FAILURE);
    }

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	if (type == from_type)
	    type = to_type;

	if (field != -1 && !Vect_cat_get(Cats, field, NULL))
	    continue;

	Vect_write_line(&Out, type, Points, Cats);
    }

    if (Vect_copy_tables(&In, &Out, 0))
        G_warning(_("Failed to copy attribute table to output map"));

    Vect_build(&Out);
    Vect_close(&Out);
    Vect_close(&In);

    exit(EXIT_SUCCESS);
}
