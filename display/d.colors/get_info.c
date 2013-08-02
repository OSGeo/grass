#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/glocale.h>
#include "colors.h"

int get_map_info(char *name, char *mapset)
{
    struct Colors colors;
    struct Categories categories;

    if (!name)
	exit(0);
    if (*name == '\0')
	exit(0);

    /* Reading color lookup table */
    if (Rast_read_cats(name, mapset, &categories) == -1)
	G_fatal_error(_("Error reading category file for <%s>"), name);

    /* Reading color lookup table */
    if (Rast_read_colors(name, mapset, &colors) == -1)
	G_fatal_error(_("Unable to read color table for raster map <%s>"), name);

    interact(&categories, &colors, name, mapset);

    /* Wrapup graphics */
    R_flush();

    return 0;
}
