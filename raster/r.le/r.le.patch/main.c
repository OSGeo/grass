/*
 ************************************************************
 * MODULE: r.le.patch/main.c                                *
 *         Version 5.0                Nov. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To analyze attributes of patches in a landscape *
 *         main.c calls user_input.c to read the user's     *
 *         requests from the screen, then displays those    *
 *         choices on screen and calls patch_fore           *
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
#include <grass/gis.h>
#include <grass/glocale.h>

#include <grass/config.h>
#define MAIN
#include "patch.h"


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
	_("Calculates attribute, patch size, core (interior) size, shape, "
	  "fractal dimension, and perimeter measures for sets of patches "
	  "in a landscape.");

    /* call user_input to read in the parameters */
    user_input(argc, argv);


    /* display the parameter choices */
    fprintf(stderr, "\nPARAMETER CHOICES:\n");
    fprintf(stderr, "\tMAP:\t  %s\n", choice->fn);
    if (choice->wrum == 'r')
	fprintf(stderr, "\tREGION:\t  %s\n", choice->reg);

    fprintf(stderr, "\tSAMPLE:");
    if (choice->wrum == 'w')
	fprintf(stderr, "\t  whole map    ");
    if (choice->wrum == 'm')
	fprintf(stderr, "\t  moving window");
    if (choice->wrum == 'u')
	fprintf(stderr, "\t  units        ");
    if (choice->wrum == 'r')
	fprintf(stderr, "\t  regions      ");

    fprintf(stderr, "\tTRACING:");
    if (choice->trace)
	fprintf(stderr, "  8 neighbor\n");
    else
	fprintf(stderr, "  4 neighbor\n");

    if (choice->coremap || choice->patchmap || choice->units)
	fprintf(stderr, "\tOUTPUT MAPS:\n");
    if (choice->coremap)
	fprintf(stderr, "\t\t  interior\n");
    if (choice->patchmap)
	fprintf(stderr, "\t\t  num\n");
    if (choice->units)
	fprintf(stderr, "\t\t  units_x\n");

    if (choice->att[0])
	fprintf(stderr, "\tATTRIBUTE MEASURES:\n");
    if (choice->att[1])
	fprintf(stderr, "\t\t  mean pixel attribute\n");
    if (choice->att[2])
	fprintf(stderr, "\t\t  st. dev. pixel attribute\n");
    if (choice->att[3])
	fprintf(stderr, "\t\t  mean patch attribute\n");
    if (choice->att[4])
	fprintf(stderr, "\t\t  st. dev. patch attribute\n");
    if (choice->att[5])
	fprintf(stderr, "\t\t  cover by gp\n");
    if (choice->att[6])
	fprintf(stderr, "\t\t  density by gp\n");
    if (choice->att[7])
	fprintf(stderr, "\t\t  total density\n");
    if (choice->att[8])
	fprintf(stderr, "\t\t  eff. mesh no.\n");

    if (choice->size[0])
	fprintf(stderr, "\tSIZE MEASURES:\n");
    if (choice->size[1])
	fprintf(stderr, "\t\t  mean patch size\n");
    if (choice->size[2])
	fprintf(stderr, "\t\t  st. dev. patch size\n");
    if (choice->size[3])
	fprintf(stderr, "\t\t  mean patch size by gp\n");
    if (choice->size[4])
	fprintf(stderr, "\t\t  st. dev. patch size by gp\n");
    if (choice->size[5])
	fprintf(stderr, "\t\t  no. by size class\n");
    if (choice->size[6])
	fprintf(stderr, "\t\t  no. by size class by gp\n");
    if (choice->size[7])
	fprintf(stderr, "\t\t  eff. mesh size\n");
    if (choice->size[8])
	fprintf(stderr, "\t\t  deg. landsc. division\n");


    if (choice->core[0])
	fprintf(stderr, "\tCORE MEASURES:\n");
    if (choice->core[1])
	fprintf(stderr, "\t\t  mean core size\n");
    if (choice->core[2])
	fprintf(stderr, "\t\t  st. dev. core size\n");
    if (choice->core[3])
	fprintf(stderr, "\t\t  mean edge size\n");
    if (choice->core[4])
	fprintf(stderr, "\t\t  st. dev. edge size\n");
    if (choice->core[5])
	fprintf(stderr, "\t\t  mean core size by gp\n");
    if (choice->core[6])
	fprintf(stderr, "\t\t  st. dev. core size by gp\n");
    if (choice->core[7])
	fprintf(stderr, "\t\t  mean edge size by gp \n");
    if (choice->core[8])
	fprintf(stderr, "\t\t  st. dev. edge size by gp\n");
    if (choice->core[9])
	fprintf(stderr, "\t\t  no. by size class \n");
    if (choice->core[10])
	fprintf(stderr, "\t\t  no. by size class by gp\n");

    if (choice->shape[0])
	fprintf(stderr, "\tSHAPE MEASURES:\n");
    if (choice->shape[1])
	fprintf(stderr, "\t\t  mean patch shape\n");
    if (choice->shape[2])
	fprintf(stderr, "\t\t  st. dev. patch shape\n");
    if (choice->shape[3])
	fprintf(stderr, "\t\t  mean patch shape by gp\n");
    if (choice->shape[4])
	fprintf(stderr, "\t\t  st. dev. patch shape by gp\n");
    if (choice->shape[5])
	fprintf(stderr, "\t\t  no. by shape class\n");
    if (choice->shape[6])
	fprintf(stderr, "\t\t  no. by shape class by gp\n");

    if (choice->boundary[0])
	fprintf(stderr, "\tBOUNDARY COMPLEXITY MEASURES:\n");
    if (choice->boundary[1])
	fprintf(stderr, "\t\t  mean twist number\n");
    if (choice->boundary[2])
	fprintf(stderr, "\t\t  st. dev. twist number\n");
    if (choice->boundary[3])
	fprintf(stderr, "\t\t  mean omega index\n");
    if (choice->boundary[4])
	fprintf(stderr, "\t\t  st. dev. omega index\n");

    if (choice->perim[0])
	fprintf(stderr, "\tPERIMETER MEASURES:\n");
    if (choice->perim[1])
	fprintf(stderr, "\t\t  sum of perims\n");
    if (choice->perim[2])
	fprintf(stderr, "\t\t  mean perim.\n");
    if (choice->perim[3])
	fprintf(stderr, "\t\t  st. dev. perim.\n");
    if (choice->perim[4])
	fprintf(stderr, "\t\t  sum of perims. by gp\n");
    if (choice->perim[5])
	fprintf(stderr, "\t\t  mean perim. by gp\n");
    if (choice->perim[6])
	fprintf(stderr, "\t\t  st. dev. perim. by gp\n");

    /* if not moving window, setup the
       r.le.out subdirectory */

    if (choice->wrum != 'm')
	G_mkdir("r.le.out");

    patch_fore();
    G_free(choice);

    return (EXIT_SUCCESS);
}
