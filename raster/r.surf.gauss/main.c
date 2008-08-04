
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
    struct Option *out;		/* Structures required for the G_parser()       */

    /* call. These can be filled with the           */
    struct Option *mean;	/* various defaults, mandatory paramters        */

    /* etc. for the GRASS user interface.           */
    struct Option *sigma;

    G_gisinit(argv[0]);		/*      This GRASS library function MUST
				   be called first to check for valid
				   database and mapset. As usual argv[0]
				   is the program name. This can be
				   recalled using G_program_name(). */

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("GRASS module to produce a raster map layer of "
	  "gaussian deviates whose mean and standard deviation "
	  "can be expressed by the user. It uses a gaussian "
	  "random number generator.");

    /****** SET PARSER OPTIONS ******/
    out = G_define_standard_option(G_OPT_R_OUTPUT);
    out->description = _("Name of the output random surface");

    mean = G_define_option();	/*      Mean of the distribution  */
    mean->key = "mean";
    mean->description = _("Distribution mean");
    mean->type = TYPE_DOUBLE;
    mean->answer = "0.0";

    sigma = G_define_option();	/*      Standard deviation of the distribution  */
    sigma->key = "sigma";
    sigma->description = _("Standard deviation");
    sigma->type = TYPE_DOUBLE;
    sigma->answer = "1.0";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    sscanf(mean->answer, "%lf", &gauss_mean);
    sscanf(sigma->answer, "%lf", &gauss_sigma);

    /****** CHECK THE CELL FILE (OUT) DOES NOT ALREADY EXIST******/
    if (G_legal_filename(out->answer) == '\0')
	G_fatal_error(_("<%s> is an illegal file name"), out->answer);

    /****** CREATE THE RANDOM CELL FILE  ******/
    gaussurf(out->answer, gauss_mean, gauss_sigma);

    exit(EXIT_SUCCESS);
}
