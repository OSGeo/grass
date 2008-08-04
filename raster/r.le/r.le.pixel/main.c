/*
 ************************************************************
 * MODULE: r.le.pixel/main.c                                *
 *         Version 5.0                Nov. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To analyze pixel-scale landscape properties     *
 *         main.c calls user_input.c, then displays user    *
 *         choices, then calls the appropriate routine      *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "pixel.h"


struct CHOICE *choice;


int main(int argc, char **argv)
{

    struct GModule *module;

    /* initialize the GRASS GIS system */
    G_gisinit(argv[0]);

    /* allocate space for the choice data structure */
    choice = (struct CHOICE *)G_calloc(1, sizeof(struct CHOICE));

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Contains a set of measures for attributes, diversity, texture, "
	  "juxtaposition, and edge.");

    /* call user_input to read in the parameters */
    user_input(argc, argv);

    /* display the parameter choices */
    fprintf(stdout, "\nPARAMETER CHOICES:\n");
    fprintf(stdout, "\tMAP:\t  %s\n", choice->fn);
    if (choice->wrum == 'r')
	fprintf(stdout, "\tREGION:\t  %s\n", choice->reg);

    fprintf(stdout, "\tSAMPLE:");
    if (choice->wrum == 'w')
	fprintf(stdout, "\t  whole map    \n");
    if (choice->wrum == 'm')
	fprintf(stdout, "\t  moving window\n");
    if (choice->wrum == 'u')
	fprintf(stdout, "\t  units        \n");
    if (choice->wrum == 'r')
	fprintf(stdout, "\t  regions      \n");

    if (choice->edgemap || choice->units || choice->z)
	fprintf(stdout, "\tOUTPUT MAPS:\n");
    if (choice->edgemap)
	fprintf(stdout, "\t\t  edge\n");
    if (choice->units)
	fprintf(stdout, "\t\t  units_x\n");
    if (choice->z)
	fprintf(stdout, "\t\t  zscores\n");

    if (choice->att[0]) {
	fprintf(stdout, "\tATTRIBUTE MEASURES:\n");
	if (choice->att[1])
	    fprintf(stdout, "\t\t  mean pixel attribute\n");
	if (choice->att[2])
	    fprintf(stdout, "\t\t  st. dev. pixel attribute\n");
	if (choice->att[3])
	    fprintf(stdout, "\t\t  minimum pixel attribute\n");
	if (choice->att[4])
	    fprintf(stdout, "\t\t  maximum pixel attribute\n");
    }

    if (choice->div[0]) {
	fprintf(stdout, "\tDIVERSITY MEASURES:\n");
	if (choice->div[1])
	    fprintf(stdout, "\t\t  richness\n");
	if (choice->div[2])
	    fprintf(stdout, "\t\t  Shannon\n");
	if (choice->div[3])
	    fprintf(stdout, "\t\t  dominance\n");
	if (choice->div[4])
	    fprintf(stdout, "\t\t  inverse Simpson\n");
    }

    if (choice->te2[0]) {
	fprintf(stdout, "\tTEXTURE METHOD:\n");
	if (choice->tex == 1)
	    fprintf(stdout, "\t\t  2N-H\n");
	else if (choice->tex == 2)
	    fprintf(stdout, "\t\t  2N-45\n");
	else if (choice->tex == 3)
	    fprintf(stdout, "\t\t  2N-V\n");
	else if (choice->tex == 4)
	    fprintf(stdout, "\t\t  2N-135\n");
	else if (choice->tex == 5)
	    fprintf(stdout, "\t\t  4N-HV\n");
	else if (choice->tex == 6)
	    fprintf(stdout, "\t\t  4N-DIAG\n");
	else if (choice->tex == 7)
	    fprintf(stdout, "\t\t  8N\n");
	fprintf(stdout, "\tTEXTURE MEASURES:\n");
	if (choice->te2[1])
	    fprintf(stdout, "\t\t  contagion\n");
	if (choice->te2[2])
	    fprintf(stdout, "\t\t  ang. sec. mom.\n");
	if (choice->te2[3])
	    fprintf(stdout, "\t\t  inv. diff. mom.\n");
	if (choice->te2[4])
	    fprintf(stdout, "\t\t  entropy\n");
	if (choice->te2[5])
	    fprintf(stdout, "\t\t  contrast\n");
    }

    if (choice->jux[0]) {
	fprintf(stdout, "\tJUXTAPOSITION MEASURES:\n");
	if (choice->jux[1])
	    fprintf(stdout, "\t\t  mean juxtaposition\n");
	if (choice->jux[2])
	    fprintf(stdout, "\t\t  standard deviation of juxtaposition\n");
    }

    if (choice->edg[0]) {
	fprintf(stdout, "\tEDGE MEASURES:\n");
	if (choice->edg[1])
	    fprintf(stdout, "\t\t  sum of edges\n");
	if (choice->edg[2])
	    fprintf(stdout, "\t\t  sum of edges by type\n");
    }

    /* if not moving window, setup the
       r.le.out subdirectory */

    if (choice->wrum != 'm')
	G_mkdir("r.le.out");

    texture_fore();
    G_free(choice);

    return (EXIT_SUCCESS);
}
