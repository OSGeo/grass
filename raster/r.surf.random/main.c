
/****************************************************************************
 *
 * MODULE:       r.surf.random
 * AUTHOR(S):    Jo Wood, 19th October, 23rd October 1991 (original contributor)
 *               Midlands Regional Research Laboratory (ASSIST)
 * AUTHOR(S):    Markus Neteler <neteler itc.it> (original contributor)
 * PURPOSE:      produces a raster map layer of uniform random deviates
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

int main(int argc, char *argv[])
{

    /****** INITIALISE ******/

    struct GModule *module;
    struct Option *out;		/* Structures required for the G_parser()       */

    /* call. These can be filled with the           */
    struct Option *min;		/* various defaults, mandatory paramters        */

    /* etc. for the GRASS user interface.           */
    struct Option *max;
    struct Flag *i_flag;

    G_gisinit(argv[0]);		/* This GRASS library function MUST
				   be called first to check for valid
				   database and mapset. As usual argv[0]
				   is the program name. This can be
				   recalled using G_program_name(). */

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Produces a raster map layer of uniform random "
	  "deviates whose range can be expressed by the user.");


    out = G_define_standard_option(G_OPT_R_OUTPUT);

    min = G_define_option();
    min->key = "min";
    min->description = _("Minimum random value");
    min->type = TYPE_INTEGER;
    min->answer = "0";

    max = G_define_option();
    max->key = "max";
    max->description = _("Maximum random value");
    max->type = TYPE_INTEGER;
    max->answer = "100";

    i_flag = G_define_flag();
    i_flag->key = 'i';
    i_flag->description = _("Create an integer map");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);	/* Returns a 0 if sucessful */


    if (G_legal_filename(out->answer) != 1)
	G_fatal_error("Illegal file name. Please try another.");


    randsurf(out->answer, atoi(min->answer), atoi(max->answer),
	     i_flag->answer);


    exit(EXIT_SUCCESS);
}
