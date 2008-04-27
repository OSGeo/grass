/****************************************************************************
 *
 * MODULE:       i.his.rgb
 *
 * AUTHOR(S):    David Satnik, GIS Laboratory, Central Washington University
 *               with acknowledgements to Ali Vali,
 *               Univ. of Texas Space Research Center, for the core routine. 
 *               
 * PURPOSE:      Hue-intensity-saturation (his) to red-green-blue (rgb)
 *               raster map color transformation function.
 *
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"

int main (int argc, char **argv)
{

    long i;
    int rows, cols;
    CELL *rowbuffer[3];
    struct Option *opt_hue, *opt_red;
    struct Option *opt_int, *opt_green;
    struct Option *opt_sat, *opt_blue;
    int fd_output[3];
    int fd_input[3];
    struct GModule *module;
    
    G_gisinit(argv[0]);
    
    /* Set description */
    module              = G_define_module();
    module->keywords = _("imagery");
    module->description = 
	_("Hue-intensity-saturation (his) to red-green-blue (rgb) "
	  "raster map color transformation function.");

    /* Define the different options */
    opt_hue = G_define_standard_option (G_OPT_R_INPUT);
    opt_hue->key           = "hue_input";
    opt_hue->description   = _("Hue map name");

    opt_int = G_define_standard_option (G_OPT_R_INPUT);
    opt_int->key           = "intensity_input";
    opt_int->description   = _("Intensity map name");

    opt_sat = G_define_standard_option (G_OPT_R_INPUT);
    opt_sat->key           = "saturation_input";
    opt_sat->description   = _("Saturation map name");

    opt_red = G_define_standard_option (G_OPT_R_OUTPUT);
    opt_red->key           = "red_output";
    opt_red->description   = _("Output map representing the red");

    opt_green = G_define_standard_option (G_OPT_R_OUTPUT);
    opt_green->key         = "green_output";
    opt_green->description = _("Output map representing the green");

    opt_blue = G_define_standard_option (G_OPT_R_OUTPUT);
    opt_blue->key          = "blue_output";
    opt_blue->description  = _("Output map representing the blue");

    if (G_parser (argc, argv) < 0)
	exit(EXIT_FAILURE);
    
    /* get dimension of the image */
    rows = G_window_rows();
    cols = G_window_cols();
    
    openfiles (opt_hue->answer, opt_int->answer, opt_sat->answer,
               opt_red->answer, opt_green->answer, opt_blue->answer,
               fd_input, fd_output, rowbuffer);
    
    for (i = 0; i < rows; i++)
    {
	int band;
	
	G_percent(i, rows, 2);

	/* read in a row from each cell map */
	for (band=0; band<3; band++)
	    if(G_get_map_row(fd_input[band], rowbuffer[band], i) < 0)
		G_fatal_error (_("Cannot read row from raster map"));

	/* process this row of the map */
	his2rgb(rowbuffer, cols);
	
	/* write out the new row for each cell map */
	for (band=0; band<3; band++)
	    if(G_put_raster_row(fd_output[band], rowbuffer[band], CELL_TYPE) < 0)
		G_fatal_error (_("Cannot write row to raster map"));
    }

    closefiles (opt_red->answer, opt_green->answer, opt_blue->answer, fd_output, rowbuffer);
    
    exit(EXIT_SUCCESS);
}
