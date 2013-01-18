
 /***************************************************************************
 *
 * MODULE:     v.out.vtk  
 * AUTHOR(S):  Soeren Gebbert
 *
 * PURPOSE:    v.out.vtk: writes ASCII VTK file
 *             this module is based on v.out.ascii
 * COPYRIGHT:  (C) 2000 by the GRASS Development Team
 *
 *             This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <string.h>

#include "local_proto.h"

double x_extent;
double y_extent;

int main(int argc, char *argv[])
{
    FILE *ascii;
    struct Option *input, *output, *type_opt, *dp_opt, *layer_opt, *scale;
    struct Flag *coorcorr, *numatts, *labels;
    int itype, *types = NULL, typenum = 0, dp, i;
    struct Map_info Map;
    struct bound_box box;
    struct GModule *module;
    int layer, level;
    double zscale = 1.0, llscale = 1.0;


    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("export"));
    module->description =
	_("Converts a vector map to VTK ASCII output.");

    input = G_define_standard_option(G_OPT_V_INPUT);

    output = G_define_standard_option(G_OPT_F_OUTPUT);
    output->required = NO;
    output->description = _("Name for output VTK file");

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->answer = "point,kernel,centroid,line,boundary,area,face";
    type_opt->options = "point,kernel,centroid,line,boundary,area,face";

    dp_opt = G_define_option();
    dp_opt->key = "dp";
    dp_opt->type = TYPE_INTEGER;
    dp_opt->required = NO;
    dp_opt->description =
	_("Number of significant digits (floating point only)");

    scale = G_define_option();
    scale->key = "scale";
    scale->type = TYPE_DOUBLE;
    scale->required = NO;
    scale->description = _("Scale factor for elevation");
    scale->answer = "1.0";

    layer_opt = G_define_option();
    layer_opt->key = "layer";
    layer_opt->type = TYPE_INTEGER;
    layer_opt->required = NO;
    layer_opt->answer = "1";
    layer_opt->description = _("Layer number");

    coorcorr = G_define_flag();                                            
    coorcorr->key = 'c';                                                   
    coorcorr->description = 
    	_("Correct the coordinates to fit the VTK-OpenGL precision");
    
    numatts = G_define_flag();
    numatts->key = 'n';
    numatts->description = 
    	_("Export numeric attribute table fields as VTK scalar variables");

    labels = NULL; /* to avoid compiler warning about "unused variable"*/
    /* not yet supported
    labels = G_define_flag();
    labels->key = 'l';
    labels->description = _("Export text attribute table fields as VTK labels");
    */
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    for (i = 0; type_opt->answers && type_opt->answers[i]; i++)
	typenum++;

    if (typenum > 0) {
	types = (int *)calloc(typenum, sizeof(int));
    }
    else {
	G_fatal_error("Usage: Wrong vector type");
    }

    i = 0;
    while (type_opt->answers[i]) {
	types[i] = -1;
	switch (type_opt->answers[i][0]) {
	case 'p':
	    types[i] = GV_POINT;
	    break;
	case 'k':
	    types[i] = GV_KERNEL;
	    break;
	case 'c':
	    types[i] = GV_CENTROID;
	    break;
	case 'l':
	    types[i] = GV_LINE;
	    break;
	case 'b':
	    types[i] = GV_BOUNDARY;
	    break;
	case 'a':
	    types[i] = GV_AREA;
	    break;
	case 'f':
	    types[i] = GV_FACE;
	    break;
	}
	i++;
    }
    itype = Vect_option_to_types(type_opt);

    /* read and compute the scale factor */
    sscanf(scale->answer, "%lf", &zscale);
    /*if LL projection, convert the elevation values to degrees */
    if (G_projection() == PROJECTION_LL) {
	llscale = M_PI / (180) * 6378137;
	zscale /= llscale;
	printf("Scale %g\n", zscale);
    }

    /*The precision of the output */
    if (dp_opt->answer) {
	if (sscanf(dp_opt->answer, "%d", &dp) != 1)
	    G_fatal_error(_("Failed to interpret 'dp' parameter as an integer"));
	if (dp > 8 || dp < 0)
	    G_fatal_error(_("dp has to be from 0 to 8"));
    }
    else {
	dp = 8;			/*This value is taken from the lib settings in G_feature_easting */
    }

    /*The Layer */
    if (layer_opt->answer) {
	if (sscanf(layer_opt->answer, "%d", &layer) != 1)
	    G_fatal_error(_("Failed to interpret 'layer' parameter as an integer"));
    }
    else {
	layer = 1;
    }

    if (output->answer) {
	ascii = fopen(output->answer, "w");
	if (ascii == NULL) {
	    G_fatal_error(_("Unable to open file <%s>"), output->answer);
	}
    }
    else {
	ascii = stdout;
    }

    /* Open input vector */
    level = Vect_open_old(&Map, input->answer, "");
    if (level < 2 && (itype & GV_AREA))
	G_fatal_error(_("Export of areas requires topology. "
	                "Please adjust '%s' option or rebuild topology."),
			type_opt->key);

    if (level == 2)
	Vect_get_map_box(&Map, &box);
    else {
	int i, type, first = TRUE;
	struct line_pnts *Points = Vect_new_line_struct();

	Vect_rewind(&Map);
	while ((type = Vect_read_next_line(&Map, Points, NULL)) > 0) {

	    if (first) {
		box.E = box.W = Points->x[0];
		box.N = box.S = Points->y[0];
		box.B = box.T = Points->z[0];
		first = FALSE;
	    }
	    for (i = 1; i < Points->n_points; i++) {
		if (Points->x[i] > box.E)
		    box.E = Points->x[i];
		else if (Points->x[i] < box.W)
		    box.W = Points->x[i];

		if (Points->y[i] > box.N)
		    box.N = Points->y[i];
		else if (Points->y[i] < box.S)
		    box.S = Points->y[i];

		if (Points->z[i] > box.T)
		    box.T = Points->z[i];
		else if (Points->z[i] < box.B)
		    box.B = Points->z[i];
	    }
	}
	Vect_destroy_line_struct(Points);
    }

    /*Correct the coordinates, so the precision of VTK is not hurt :( */
    if (coorcorr->answer) {

	/*Use the center of the vector's bbox as extent */
	y_extent = (box.N + box.S) / 2;
	x_extent = (box.W + box.E) / 2;
    }
    else {
	x_extent = 0;
	y_extent = 0;
    }

    /*Write the header */
    write_vtk_head(ascii, &Map);
    /*Write the geometry and data */
    write_vtk(ascii, &Map, layer, types, typenum, dp, zscale, numatts->answer, 0 );
    /* change to this, when labels get supported:
    write_vtk(ascii, &Map, layer, types, typenum, dp, zscale, numatts->answer, labels->answer );
    */

    if (ascii != NULL)
	fclose(ascii);

    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}
