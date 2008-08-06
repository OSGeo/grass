#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/edit.h>


/*
 ***********************************************************************
 *
 * MODULE:        modhist (etc/support) for r.support
 *
 * AUTHOR(S):     Original by Michael Shapiro - CERL
 *                Port to 6.x by Brad Douglas
 *
 * PURPOSE:       Allows editing of raster map history
 *
 * COPYRIGHT:     (C) 2000-2005 by the GRASS Development Team
 *
 *                This program is free software under the GNU General
 *                Public License (>=v2). Read the file COPYING that
 *                comes with GRASS for details.
 *
 ************************************************************************/

int main(int argc, char *argv[])
{
    char name[64];
    char *mapset;
    struct History hist;


    /* Initialize GIS engine */
    G_gisinit(argv[0]);

    if (argc < 2) {
	mapset =
	    G_ask_cell_in_mapset(_("Which raster map needs an updated history? "),
				 name);
	if (mapset == NULL)
	    return EXIT_SUCCESS;
    }
    else {
	strncpy(name, argv[1], sizeof(name));
	if ((mapset = G_find_cell2(name, G_mapset())) == NULL)
	    G_fatal_error(_("Raster file [%s] not found. Exiting."), argv[1]);
    }

    G_read_history(name, mapset, &hist);
    if (E_edit_history(&hist) > 0 && G_write_history(name, &hist) >= 0)
	G_message(_("History file for [%s] updated."), name);
    else
	G_message(_("History file for [%s] not updated."), name);

    return EXIT_SUCCESS;
}
