
/****************************************************************************
 *
 * MODULE:       v.to.3d
 *
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com>
 *               
 * PURPOSE:      Performs transformation of 2D vector features to 3D
 *
 * COPYRIGHT:    (C) 2008-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char **argv)
{
    struct GModule *module;
    struct opts opt;
    struct Map_info In, Out;
    struct bound_box box;
    int type;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("3D"));
    module->description =
	_("Performs transformation of 2D vector features to 3D.");

    parse_args(&opt);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    
    type = Vect_option_to_types(opt.type);

    if (!opt.reverse->answer) {
	if ((!opt.height->answer && !opt.column->answer) ||
	    (opt.height->answer && opt.column->answer)) {
	    G_fatal_error(_("Either '%s' or '%s' parameter have to be used"),
			  opt.height->key, opt.column->key);
	}
    }
    else {
	if (opt.height->answer) {
	    G_warning(_("Parameters '%s' ignored"), opt.height->key);
	}
    }

    if (opt.reverse->answer && opt.table->answer) {
	G_fatal_error(_("Attribute table required"));
    }

    Vect_check_input_output_name(opt.input->answer, opt.output->answer,
				 G_FATAL_EXIT);
    
    /* open input vector, topology not needed */
    Vect_set_open_level(1);
    if (Vect_open_old2(&In, opt.input->answer, "", opt.field->answer) < 1)
	G_fatal_error(_("Unable to open vector map <%s>"), opt.input->answer);
    Vect_set_error_handler_io(&In, &Out);
    
    if (opt.reverse->answer && !Vect_is_3d(&In)) {
	G_fatal_error(_("Vector map <%s> is 2D"), opt.input->answer);
    }

    if (!opt.reverse->answer && Vect_is_3d(&In)) {
	G_fatal_error(_("Vector map <%s> is 3D"), opt.input->answer);
    }

    /* create output vector */
    Vect_set_open_level(2);
    if (Vect_open_new(&Out, opt.output->answer,
		      opt.reverse->answer ? WITHOUT_Z : WITH_Z) == -1)
	G_fatal_error(_("Unable to create vector map <%s>"),
		      opt.output->answer);

    /* copy history & header */
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);
    Vect_copy_head_data(&In, &Out);

    if (opt.reverse->answer && !opt.table->answer) {
	G_message(_("Copying attributes..."));
	if (Vect_copy_tables(&In, &Out, 0) == -1) {
	    G_warning(_("Unable to copy attributes"));
	}
    }

    if (opt.reverse->answer) {
	/* 3d -> 2d */
        trans3d(&In, &Out, type, opt.field->answer, opt.column->answer);
    }
    else {
	/* 2d -> 3d */
	double height = 0.;

	if (opt.height->answer) {
	    height = atof(opt.height->answer);
	}
	trans2d(&In, &Out, type, height, opt.field->answer, opt.column->answer);
    }

    if (!opt.reverse->answer && !opt.table->answer) {
	G_message(_("Copying attributes..."));
	if (Vect_copy_tables(&In, &Out, 0) == -1) {
	    G_warning(_("Unable to copy attributes"));
	}
    }

    Vect_close(&In);
    Vect_build(&Out);

    if (!opt.reverse->answer) {
	Vect_get_map_box(&Out, &box);
	G_message(_("Vertical extent of vector map <%s>: B: %f T: %f"),
		  opt.output->answer, box.B, box.T);
    }

    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
