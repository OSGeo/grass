/***************************************************************
 *
 * MODULE:       v.net
 * 
 * AUTHOR(S):    Radim Blazek
 *               Operation 'connect' added by Martin Landa
 *                <landa.martin gmail.com>, 2007/07
 *               
 * PURPOSE:      Network maintenance
 *               
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
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
#include <grass/glocale.h>
#include <grass/Vect.h>
#include "proto.h"

int main (int argc, char **argv)
{
    struct GModule *module;
    struct Option *input, *points;
    struct Option *output;
    struct Option *action;
    struct Option *afield_opt, *nfield_opt, *thresh_opt;
    struct Flag *cats_flag;
    struct Map_info In, Out, Points;

    char   *mapset;
    int    afield, nfield;
    int    act;
    double thresh;
	    
    /*  Initialize the GIS calls */
    G_gisinit(argv[0]) ;

    module = G_define_module();
    module->keywords = _("vector, networking");
    module->description = _("Network maintenance.");

    /* Define the options */
    input = G_define_standard_option (G_OPT_V_INPUT);

    points = G_define_standard_option (G_OPT_V_INPUT);
    points->key = "points";
    points->label = _("Name of input point vector map");
    points->description = _("Required for operation 'connect'");
    points->required = NO;

    output = G_define_standard_option (G_OPT_V_OUTPUT);
    output->required = NO;

    action = G_define_option ();
    action->key = "operation";
    action->type = TYPE_STRING;
    action->required = NO;
    action->multiple = NO;
    action->answer = "nodes";
    action->options = "nodes,connect,report,nreport";
    action->description = _("Operation to be performed");
    action->descriptions = _("nodes;new point is placed on each node (line end) "
			     "if doesn't exist;"
			     "connect;connect still unconnected points to vector network "
			     "by inserting new line(s);"
			     "report;print to standard output "
			     "{line_category start_point_category end_point_category};"
			     "nreport;print to standard output "
			     "{point_category line_category[,line_category...]}");

    afield_opt = G_define_standard_option(G_OPT_V_FIELD);
    afield_opt->key = "alayer";
    afield_opt->label = _("Arc layer (network)");

    nfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    nfield_opt->key = "nlayer";
    nfield_opt->answer = "2";
    nfield_opt->label = _("Node layer (points)");    

    thresh_opt = G_define_option();
    thresh_opt ->key = "thresh";
    thresh_opt ->type =  TYPE_DOUBLE;
    thresh_opt ->required = NO;
    thresh_opt ->multiple = NO;
    thresh_opt ->label       = "Threshold";
    thresh_opt ->description = _("Required for operation 'connect'. Connect points in given threshold.");

    cats_flag = G_define_flag();
    cats_flag->key = 'c';
    cats_flag->label = _("Assign unique categories to new points");
    cats_flag->description = _("For operation 'nodes'");
  
    if (G_parser(argc, argv))
        exit (EXIT_FAILURE);
  
    afield = atoi (afield_opt->answer);
    nfield = atoi (nfield_opt->answer);
    thresh = 0.0;

    if (strcmp ( action->answer, "nodes") == 0)
	act = TOOL_NODES;
    else if (strcmp ( action->answer, "connect") == 0)
	act = TOOL_CONNECT;
    else if (strcmp ( action->answer, "report") == 0)
	act = TOOL_REPORT;
    else if (strcmp ( action->answer, "nreport") == 0)
	act = TOOL_NREPORT;
    else 
	G_fatal_error(_("Unknow operation"));

    if (act == TOOL_NODES || act == TOOL_CONNECT) {
        if (output->answer == NULL) 
	    G_fatal_error(_("Output vector map must be specified"));
    }
    
    if (act == TOOL_CONNECT) {
	if (points->answer == NULL) 
	    G_fatal_error(_("Point vector map must be specified"));
	
	if (thresh_opt->answer == NULL) 
	    G_fatal_error(_("Threshold value must be specified"));

	thresh = atof (thresh_opt->answer);
	
	if (thresh < 0.0)
	    G_fatal_error (_("Threshold value must be >= 0"));
    }

    /* open input map */
    mapset = G_find_vector2 (input->answer, "");
    if (mapset == NULL) 
	G_fatal_error(_("Vector map <%s> not found"), input->answer);
    
    Vect_set_open_level (2);
    Vect_open_old (&In, input->answer, mapset);
    
    if (act == TOOL_NODES || act == TOOL_CONNECT) { /* nodes */
	int is3d;

	Vect_check_input_output_name ( input->answer, output->answer, GV_FATAL_EXIT );

	if (act == TOOL_CONNECT) {
	    /* open points map */
	    mapset = G_find_vector2 (points->answer, "");
	    if (mapset == NULL) 
		G_fatal_error(_("Vector map <%s> not found"), points->answer);
	    
	    Vect_set_open_level (1);
	    Vect_set_fatal_error (GV_FATAL_PRINT);
	    if (Vect_open_old (&Points, points->answer, mapset) == -1) {
		Vect_close(&In);
		G_fatal_error (_("Unable to open vector map <%s>"), points->answer);
	    }
	}

	/* create output map */
	is3d = Vect_is_3d (&In);
	Vect_set_fatal_error (GV_FATAL_PRINT);
	if (1 > Vect_open_new (&Out, output->answer, is3d)) {
	    Vect_close (&In);
	    G_fatal_error (_("Unable to open vector map <%s> at topology level %d"), output->answer, 2);
	}

	Vect_copy_head_data (&In, &Out);
	Vect_hist_copy (&In, &Out);
	Vect_hist_command (&Out);

	if (act == TOOL_NODES) {
	    nodes (&In, &Out, cats_flag->answer, nfield);
	}
	else { /* TOOL_CONNECT */
	    int narcs;

	    narcs = connect_arcs (&In, &Points, &Out, nfield, thresh);

	    G_message (_("%d arcs added to network (nlayer: [%d])"), narcs, nfield);

	    Vect_close(&Points);
	}

	Vect_copy_tables (&In, &Out, 0);
	
	/* support */
	Vect_build_partial (&Out, GV_BUILD_NONE, NULL);
	if (G_verbose() > G_verbose_min())
	    Vect_build (&Out, stderr); 
	else
	    Vect_build (&Out, NULL); 
    
	Vect_close (&In);
	Vect_close (&Out);
    }
    else  /* report */
    {
	report (&In, afield, nfield, act);
    }

  return (EXIT_SUCCESS);
}
