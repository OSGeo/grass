#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/*
 **********************************************************************
 *
 * MODULE:        modcolr (etc/support) for r.support
 *
 * AUTHOR(S):     Original by Michael Shapiro - CERL
 *                Ported to 6.x by Brad Douglas
 *
 * PURPOSE:       modify raster color tables
 *
 * COPYRIGHT:     (C) 2000-2005 by the GRASS Development Team
 *
 *                This program is free software under the GNU General
 *                Purpose License (>=v2). Read the file COPYING that
 *                comes with GRASS for details.
 *
 ***********************************************************************/

int main(int argc, char *argv[])
{
    int stat;
    char name[64];
    char *mapset;
    struct Colors pcolr;


    /* Initialize GIS engine */
    G_gisinit(argv[0]);

    if (argc < 2) {
	mapset =
	    G_ask_cell_old(_("Which raster map needs a color table"), name);
	if (mapset == NULL)
	    return EXIT_SUCCESS;
    }
    else {
	strncpy(name, argv[1], sizeof(name));
	if ((mapset = G_find_cell2(name, "")) == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), name);
    }

    if ((stat = G_ask_colors(name, mapset, &pcolr)) >= 0) {
	if (stat > 0)
	    stat = G_write_colors(name, mapset, &pcolr);

	if (stat >= 0)
	    G_message(_("Color table for <%s> updated"), name);
    }

    /* Free resources */
    G_free_colors(&pcolr);

    return EXIT_SUCCESS;
}
