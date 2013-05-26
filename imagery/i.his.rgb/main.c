
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
 * COPYRIGHT:    (C) 2007-2008 by the GRASS Development Team
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
#include <grass/raster.h>
#include <grass/glocale.h>
#include "globals.h"

int main(int argc, char **argv)
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
    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("color transformation"));
    G_add_keyword("RGB");
    G_add_keyword("HIS");
    G_add_keyword("IHS");
    module->description =
	_("Transforms raster maps from HIS (Hue-Intensity-Saturation) color space to "
	  "RGB (Red-Green-Blue) color space.");

    /* Define the different options */
    opt_hue = G_define_standard_option(G_OPT_R_INPUT);
    opt_hue->key = "hue_input";
    opt_hue->description = _("Name of input raster map (hue)");

    opt_int = G_define_standard_option(G_OPT_R_INPUT);
    opt_int->key = "intensity_input";
    opt_int->description = _("Name of input raster map (intensity)");

    opt_sat = G_define_standard_option(G_OPT_R_INPUT);
    opt_sat->key = "saturation_input";
    opt_sat->description = _("Name of input raster map (saturation)");

    opt_red = G_define_standard_option(G_OPT_R_OUTPUT);
    opt_red->key = "red_output";
    opt_red->description = _("Name for output raster map (red)");

    opt_green = G_define_standard_option(G_OPT_R_OUTPUT);
    opt_green->key = "green_output";
    opt_green->description = _("Name for output raster map (green)");

    opt_blue = G_define_standard_option(G_OPT_R_OUTPUT);
    opt_blue->key = "blue_output";
    opt_blue->description = _("Name for output raster map (blue)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* get dimension of the image */
    rows = Rast_window_rows();
    cols = Rast_window_cols();

    openfiles(opt_hue->answer, opt_int->answer, opt_sat->answer,
	      opt_red->answer, opt_green->answer, opt_blue->answer,
	      fd_input, fd_output, rowbuffer);

    for (i = 0; i < rows; i++) {
	int band;

	G_percent(i, rows, 2);

	/* read in a row from each cell map */
	for (band = 0; band < 3; band++)
	    Rast_get_c_row(fd_input[band], rowbuffer[band], i);

	/* process this row of the map */
	his2rgb(rowbuffer, cols);

	/* write out the new row for each cell map */
	for (band = 0; band < 3; band++)
	    Rast_put_row(fd_output[band], rowbuffer[band], CELL_TYPE);
    }

    closefiles(opt_red->answer, opt_green->answer, opt_blue->answer,
	       fd_output, rowbuffer);

    exit(EXIT_SUCCESS);
}
