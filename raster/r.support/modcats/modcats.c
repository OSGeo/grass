#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/edit.h>


/*
 **********************************************************************
 *
 * MODULE:        modcats (etc/support) for r.support
 *
 * AUTHOR(S):     Original by Michael Shapiro - CERL
 *                Port to 6.x by Brad Douglas
 *
 * PURPOSE:       Modify raster categories
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
    char name[128];
    char *mapset;
    struct Categories cats;
    int vector = 0;
    int stat;


    /* Initialize GIS engine */
    G_gisinit(argv[0]);

    if (argc > 1 && (strcmp(argv[1], "-v") == 0)) {
	vector = 1;
	argc--;
	argv++;
    }

    if (argc < 2) {
	if (vector)
	    mapset = G_ask_vector_in_mapset(_("Which vector map needs "
					      "updated categories?"), name);
	else
	    mapset = G_ask_cell_in_mapset(_("Which raster map needs "
					    "updated categories?"), name);

	if (mapset == NULL)
	    G_fatal_error(_("%s map <%s> not found"),
			  vector ? "Vector" : "Raster", name);
    }
    else {
	strncpy(name, argv[1], sizeof(name));
	mapset = (vector) ? G_find_vector2(name, G_mapset()) :
	    G_find_cell2(name, G_mapset());

	if (mapset == NULL)
	    G_fatal_error(_("%s map <%s> not found"),
			  vector ? "Vector" : "Raster", name);
    }

    stat = (vector) ? G_read_vector_cats(name, mapset, &cats) :
	G_read_cats(name, mapset, &cats);

    if (stat < 0)
	G_init_cats((CELL) 0, "", &cats);

    if (!vector && G_raster_map_is_fp(name, mapset)) {
	if (E_edit_fp_cats(name, &cats) < 0) {
	    G_message(_("Category file for <%s> not updated"), name);

	    return EXIT_SUCCESS;
	}
    }
    else {
	if (E_edit_cats(name, &cats, stat < 0) < 0) {
	    G_message(_("Category file for <%s> not updated"), name);

	    return EXIT_SUCCESS;
	}
    }

    if (vector)
	G_write_vector_cats(name, &cats);
    else
	G_write_cats(name, &cats);

    G_message(_("Category file for <%s> updated"), name);

    return EXIT_SUCCESS;
}
