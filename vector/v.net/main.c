
/***************************************************************
 *
 * MODULE:       v.net
 * 
 * AUTHOR(S):    Radim Blazek
 *               Martin Landa <landa.martin gmail.com> (connect/arcs)
 *               
 * PURPOSE:      Network maintenance
 *               
 * COPYRIGHT:    (C) 2001-2009 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "proto.h"

int main(int argc, char **argv)
{
    struct GModule *module;
    struct opt opt;
    struct Map_info *In = NULL, *Out = NULL, *Points = NULL;

    FILE *file_arcs;

    int afield, nfield;
    int act;
    double thresh;

    char message[4096];
    
    /*  initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("network"));
    G_add_keyword(_("network maintenance"));
    module->description = _("Performs network maintenance.");

    define_options(&opt);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    parse_arguments(&opt, &afield, &nfield, &thresh, &act);

    In = Points = Out = NULL;
    file_arcs = NULL;
    message[0] = '\0';
    
    /* open input map */
    if (act != TOOL_ARCS) {
	In = (struct Map_info *)G_malloc(sizeof(struct Map_info));
	Vect_set_open_level(2);
	if (Vect_open_old(In, opt.input->answer, "") == -1)
	    G_fatal_error(_("Unable to open vector map <%s>"),
			  opt.input->answer);
    }

    if (act == TOOL_NODES || act == TOOL_CONNECT || act == TOOL_ARCS) {
	int is3d;

	/* non-report operations */
	if (act != TOOL_ARCS) {
	    /* check input-output */
	    Vect_check_input_output_name(opt.input->answer,
					 opt.output->answer, G_FATAL_EXIT);
	}

	if (act == TOOL_CONNECT || act == TOOL_ARCS) {
	    /* open points map */
	    Points = (struct Map_info *)G_malloc(sizeof(struct Map_info));
	    if (act == TOOL_CONNECT)
		Vect_set_open_level(1);
	    else
		Vect_set_open_level(2);
	    if (Vect_open_old(Points, opt.points->answer, "") == -1) {
		if (In)
		    Vect_close(In);
		G_fatal_error(_("Unable to open vector map <%s>"),
			      opt.points->answer);
	    }

	    if (act == TOOL_ARCS) {
		/* open input file */
		if (strcmp(opt.file->answer, "-")) {
		    if ((file_arcs = fopen(opt.file->answer, "r")) == NULL) {
			G_fatal_error(_("Unable to open file <%s>"),
				      opt.file->answer);
		    }
		}
		else {
		    file_arcs = stdin;
		}
	    }
	}

	/* create output map */
	Out = (struct Map_info *)G_malloc(sizeof(struct Map_info));
	is3d = WITHOUT_Z;
	if (In)
	    is3d = Vect_is_3d(In);
	else if (Points)
	    is3d = Vect_is_3d(Points);

	if (Vect_open_new(Out, opt.output->answer, is3d) == -1) {
	    if (In)
		Vect_close(In);
	    G_fatal_error(_("Unable to open vector map <%s> at topology level %d"),
			  opt.output->answer, 2);
	}

	/* copy header */
	if (In) {
	    Vect_copy_head_data(In, Out);
	    Vect_hist_copy(In, Out);
	}
	Vect_hist_command(Out);

	if (act == TOOL_NODES) {
	    /* nodes */
	    int nnodes;
	    nnodes = nodes(In, Out, opt.cats_flag->answer, nfield);

	    sprintf (message, _("%d new points (nodes) written to output."), nnodes);
	}
	else {			/* connect or arcs */
	    int narcs;

	    if (act == TOOL_CONNECT)
		narcs = connect_arcs(In, Points, Out, afield, nfield,
		                     thresh, opt.snap_flag->answer);
	    else
		narcs = create_arcs(file_arcs, Points, Out, afield, nfield);

	    sprintf(message, _("%d lines (network arcs) written to output."), narcs);
	}

	if (In) {
	  G_message (_("Copying attributes..."));
	  if (Vect_copy_tables(In, Out, 0))
	    G_warning(_("Failed to copy attribute table to output map"));
	}
	
	/* support */
	Vect_build_partial(Out, GV_BUILD_NONE);
	Vect_build(Out);

	if (Points)
	    Vect_close(Points);
	if (Out)
	    Vect_close(Out);
    }
    else {			/* report */
	report(In, afield, nfield, act);
    }

    if (In)
	Vect_close(In);

    if (file_arcs)
	fclose(file_arcs);

    G_done_msg(message);

    return (EXIT_SUCCESS);
}
