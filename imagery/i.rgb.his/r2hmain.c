/****************************************************************************
 *
 * MODULE:       i.rgb.his
 *
 * AUTHOR(S):    David Satnik, GIS Laboratory, Central Washington University
 *               with acknowledgements to Ali Vali,
 *               Univ. of Texas Space Research Center, for the core routine. 
 *               
 * PURPOSE:      Red-green-blue (rgb) to hue-intensity-saturation (his) 
 *               raster map color transformation function
 *
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"

int main (int argc, char **argv)
{
    long i;
    int band, rows, cols;
    CELL *rowbuffer[3];
    struct Option *opt_hue, *opt_red;
    struct Option *opt_inten, *opt_green;
    struct Option *opt_sat, *opt_blue;
    struct GModule *module;
    int fd_input[3];
    int fd_output[3];
    
    /* Initialize GIS engine */
    G_gisinit(argv[0]);
    
    /* Set description */
    module              = G_define_module();
    module->keywords = _("imagery");
    module->description = 
	_("Red-green-blue (rgb) to hue-intensity-saturation (his) raster "
	  "map color transformation function");
    
    /* Define the different options */
    opt_red = G_define_standard_option (G_OPT_R_INPUT); 
    opt_red->key           = "red_input";
    opt_red->description   = _("Red input raster map");

    opt_green = G_define_standard_option (G_OPT_R_INPUT);
    opt_green->key         = "green_input";
    opt_green->description = _("Green input raster map");

    opt_blue = G_define_standard_option (G_OPT_R_INPUT);
    opt_blue->key          = "blue_input";
    opt_blue->description  = _("Blue input raster map");

    opt_hue = G_define_standard_option (G_OPT_R_OUTPUT);
    opt_hue->key           = "hue_output";
    opt_hue->description   = _("Output hue raster map");

    opt_inten = G_define_standard_option (G_OPT_R_OUTPUT);
    opt_inten->key         = "intensity_output";
    opt_inten->description = _("Output intensity raster map");

    opt_sat = G_define_standard_option (G_OPT_R_OUTPUT);
    opt_sat->key           = "saturation_output";
    opt_sat->description   = _("Output saturation raster map");

    if (G_parser (argc, argv) < 0)
        exit (EXIT_FAILURE);

    /* get dimension of the image */
    rows = G_window_rows();
    cols = G_window_cols();
    
    openfiles (opt_red->answer, opt_green->answer, opt_blue->answer,
               opt_hue->answer, opt_inten->answer, opt_sat->answer,
               fd_input, fd_output, rowbuffer);
    
    for (i=0; i<rows; i++) {
	/* read in a row from each cell map */
	G_percent(i, rows, 2);
	
	for (band = 0; band < 3; band++)
	    if(G_get_map_row(fd_input[band], rowbuffer[band], i) < 0)
		G_fatal_error (_("Cannot read row from raster map"));
	
	/* process this row of the map */
	rgb2his(rowbuffer,cols);
	
	/* write out the new row for each cell map */
	for (band = 0; band < 3; band++)
	    if (G_put_raster_row (fd_output[band], rowbuffer[band], CELL_TYPE) < 0)
		G_fatal_error (_("Cannot write row to raster map"));
    }

    closefiles (opt_hue->answer, opt_inten->answer, opt_sat->answer,
                fd_output, rowbuffer);

    exit (EXIT_SUCCESS);
}
