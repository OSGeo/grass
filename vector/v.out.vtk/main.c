
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
#include <grass/Vect.h>
#include <grass/glocale.h>
#include <string.h>

#define MAIN
#include "local_proto.h"

int main(int argc, char *argv[])
{
    FILE *ascii;
    struct Option *input, *output, *type_opt, *dp_opt, *layer_opt, *scale;
    struct Flag *coorcorr;
    int *types = NULL, typenum = 0, dp, i;
    struct Map_info Map;
    struct GModule *module;
    int layer;
    struct Cell_head region;
    double zscale = 1.0, llscale = 1.0;
   
    
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector");
    module->description =
	_("Converts a GRASS binary vector map to VTK ASCII output.");

    input = G_define_standard_option(G_OPT_V_INPUT);

    output = G_define_option();
    output->key = "output";
    output->type = TYPE_STRING;
    output->required = NO;
    output->multiple = NO;
    output->gisprompt = "new_file,file,output";
    output->description = _("Path to resulting VTK file");

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
    coorcorr->description = _("Correct the coordinates to fit the VTK-OpenGL precision");
    
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

    G_get_set_window(&region);

    /*Correct the coordinates, so the precision of VTK is not hurt :( */
    if(coorcorr->answer){
       /*Get the default region for coordiante correction*/
       G_get_default_window(&region);

       /*Use the center of the current region as extent*/
       y_extent = (region.north + region.south)/2;
       x_extent = (region.west + region.east)/2;
    } else
    {
       x_extent = 0;
       y_extent = 0;
    }


    /* read and compute the scale factor*/
    sscanf(scale->answer, "%lf", &zscale);
    /*if LL projection, convert the elevation values to degrees*/
    if(region.proj == PROJECTION_LL) {
      llscale = M_PI/(180)*6378137;
      zscale /= llscale;
      printf("Scale %g\n", zscale);
    }

    /*We need level 2 functions */
    Vect_set_open_level(2);
    Vect_open_old(&Map, input->answer, "");

    if (output->answer) {
	ascii = fopen(output->answer, "w");
	if (ascii == NULL) {
	    G_fatal_error(_("Unable to open file <%s>"), output->answer);
	}
    }
    else {
	ascii = stdout;
    }

    /*The precision of the output */
    if (dp_opt->answer) {
	if (sscanf(dp_opt->answer, "%d", &dp) != 1)
	    G_fatal_error(_("Failed to interprete 'dp' parameter as an integer"));
	if (dp > 8 || dp < 0)
	    G_fatal_error(_("dp has to be from 0 to 8"));
    }
    else {
	dp = 8;			/*This value is taken from the lib settings in G_feature_easting */
    }

    /*The Layer */
    if (layer_opt->answer) {
	if (sscanf(layer_opt->answer, "%d", &layer) != 1)
	    G_fatal_error(_("Failed to interprete 'layer' parameter as an integer"));
    }
    else {
	layer = 1;
    }

    /*Write the header */
    write_vtk_head(ascii, &Map);
    /*Write the geometry and data */
    write_vtk(ascii, &Map, layer, types, typenum, dp, zscale);

    if (ascii != NULL)
	fclose(ascii);

    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}

