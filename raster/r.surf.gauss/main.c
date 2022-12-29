
/****************************************************************************
 *
 * MODULE:       r.surf.gauss
 * AUTHOR(S):    Jo Wood, 19th October, 24th October 1991 (original contributor)
 *               Midlands Regional Research Laboratory (ASSIST)
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/glocale.h>
#include "local_proto.h"


int main(int argc, char *argv[])
{

    /****** INITIALISE ******/
    double gauss_mean, gauss_sigma;

    struct GModule *module;
    struct Option *out;
    struct Option *mean;
    struct Option *sigma;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("random"));
    module->label =
	_("Generates a raster map using gaussian "
	  "random number generator.");
    module->description = _("Mean and standard deviation of gaussian deviates "
			    "can be expressed by the user.");
    
    out = G_define_standard_option(G_OPT_R_OUTPUT);

    mean = G_define_option();
    mean->key = "mean";
    mean->description = _("Distribution mean");
    mean->type = TYPE_DOUBLE;
    mean->answer = "0.0";

    sigma = G_define_option();
    sigma->key = "sigma";
    sigma->description = _("Standard deviation");
    sigma->type = TYPE_DOUBLE;
    sigma->answer = "1.0";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    sscanf(mean->answer, "%lf", &gauss_mean);
    sscanf(sigma->answer, "%lf", &gauss_sigma);

    gaussurf(out->answer, gauss_mean, gauss_sigma);

    exit(EXIT_SUCCESS);
}
