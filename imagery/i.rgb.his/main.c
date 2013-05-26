
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
 * COPYRIGHT:    (C) 2007-2008 by the GRASS Development Team
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
#include <grass/raster.h>
#include <grass/glocale.h>
#include "globals.h"

int main(int argc, char **argv)
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
    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("color transformation"));
    G_add_keyword("RGB");
    G_add_keyword("HIS");
    G_add_keyword("IHS");
    module->description =
	_("Transforms raster maps from RGB (Red-Green-Blue) color space to "
	  "HIS (Hue-Intensity-Saturation) color space.");

    /* Define the different options */
    opt_red = G_define_standard_option(G_OPT_R_INPUT);
    opt_red->key = "red_input";
    opt_red->description = _("Name of input raster map (red)");

    opt_green = G_define_standard_option(G_OPT_R_INPUT);
    opt_green->key = "green_input";
    opt_green->description = _("Name of input raster map (green)");

    opt_blue = G_define_standard_option(G_OPT_R_INPUT);
    opt_blue->key = "blue_input";
    opt_blue->description = _("Name of input raster map (blue)");

    opt_hue = G_define_standard_option(G_OPT_R_OUTPUT);
    opt_hue->key = "hue_output";
    opt_hue->description = _("Name for output raster map (hue)");

    opt_inten = G_define_standard_option(G_OPT_R_OUTPUT);
    opt_inten->key = "intensity_output";
    opt_inten->description = _("Name for output raster map (intensity)");

    opt_sat = G_define_standard_option(G_OPT_R_OUTPUT);
    opt_sat->key = "saturation_output";
    opt_sat->description = _("Name for output raster map (saturation)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* get dimension of the image */
    rows = Rast_window_rows();
    cols = Rast_window_cols();

    openfiles(opt_red->answer, opt_green->answer, opt_blue->answer,
	      opt_hue->answer, opt_inten->answer, opt_sat->answer,
	      fd_input, fd_output, rowbuffer);

    for (i = 0; i < rows; i++) {
	/* read in a row from each cell map */
	G_percent(i, rows, 2);

	for (band = 0; band < 3; band++)
	    Rast_get_c_row(fd_input[band], rowbuffer[band], i);

	/* process this row of the map */
	rgb2his(rowbuffer, cols);

	/* write out the new row for each cell map */
	for (band = 0; band < 3; band++)
	    Rast_put_row(fd_output[band], rowbuffer[band], CELL_TYPE);
    }
    G_percent(i, rows, 2);

    closefiles(opt_hue->answer, opt_inten->answer, opt_sat->answer,
	       fd_output, rowbuffer);


    exit(EXIT_SUCCESS);
}
