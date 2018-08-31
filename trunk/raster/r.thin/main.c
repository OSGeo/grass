
/****************************************************************************
 *
 * MODULE:       r.thin
 * AUTHOR(S):    Olga Waupotitsch, CERL (original contributor)
 *               The code for finding the bounding box as well as
 *               input/output code was written by Mike Baba (DBA
 *               Systems, 1990) and Jean Ezell (USACERL, 1988).
 *
 *               Roberto Flor <flor itc.it>, Markus Neteler <neteler itc.it>
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Jan-Oliver Wagner <jan intevation.de>,
 *               Huidae Cho <grass4u gmail.com>
 * PURPOSE:      Cell-file line thinning
 * COPYRIGHT:    (C) 1999-2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/* Cell-file line thinning */

/* Mike Baba */
/* DBA Systems */
/* Fairfax, Va */
/* Jan 1990 */

/* Jean Ezell */
/* US Army Corps of Engineers */
/* Construction Engineering Research Lab */
/* Modelling and Simulation Team */
/* Champaign, IL  61820 */
/* January - February 1988 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "local_proto.h"
#include <grass/glocale.h>


int main(int argc, char *argv[])
{
    char *input, *output;
    struct GModule *module;
    struct Option *opt1, *opt2, *opt3;
    struct History history;
    int iterations;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("geometry"));
    module->description =
	_("Thins non-null cells that denote linear "
	  "features in a raster map layer.");

    opt1 = G_define_standard_option(G_OPT_R_INPUT);

    opt2 = G_define_standard_option(G_OPT_R_OUTPUT);

    opt3 = G_define_option();
    opt3->key = "iterations";
    opt3->type = TYPE_INTEGER;
    opt3->required = NO;
    opt3->answer = "200";
    opt3->description = _("Maximal number of iterations");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    input = opt1->answer;
    output = opt2->answer;
    iterations = atoi(opt3->answer);

    open_file(input);
    thin_lines(iterations);
    close_file(output);

    Rast_put_cell_title(output, "Thinned linear features");
    Rast_short_history(output, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(output, &history);

    exit(EXIT_SUCCESS);
}
