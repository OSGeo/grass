#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/colors.h>
#include "colors.h"

int get_map_info(char *name, char *mapset)
{
    struct Colors colors;
    struct Categories categories;
    char buff[128];

    if (!name)
	exit(0);
    if (*name == '\0')
	exit(0);

    /* Reading color lookup table */
    if (G_read_cats(name, mapset, &categories) == -1) {
	sprintf(buff, "category file for [%s] not available", name);
	G_fatal_error(buff);
    }

    /* Reading color lookup table */
    if (G_read_colors(name, mapset, &colors) == -1) {
	sprintf(buff, "color file for [%s] not available", name);
	G_fatal_error(buff);
    }

    interact(&categories, &colors, name, mapset);

    /* Wrapup graphics */
    R_flush();

    return 0;
}
