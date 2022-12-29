/*!
   \file lib/gis/color_str.c

   \brief GIS library - color management, named color to RGB triplet

   (C) 2001-2016 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Original author CERL
 */

#include <string.h>

#include <grass/gis.h>
#include <grass/colors.h>

/* The order in this table is important! It will be indexed by color number */
static const struct color_rgb standard_colors_rgb[] = {
    {0, 0, 0},			/* This is a dummy value to make lookup easier */
    {0, 0, 0},			/* BLACK   */
    {255, 0, 0},		/* RED     */
    {0, 255, 0},		/* GREEN   */
    {0, 0, 255},		/* BLUE    */
    {255, 255, 0},		/* YELLOW  */
    {0, 255, 255},		/* CYAN    */
    {255, 0, 255},		/* MAGENTA */
    {255, 255, 255},		/* WHITE   */
    {128, 128, 128},		/* GRAY    */
    {255, 128, 0},		/* ORANGE  */
    {100, 128, 255},		/* AQUA    */
    {0, 128, 255},		/* INDIGO  */
    {128, 0, 255},		/* VIOLET  */
    {180, 77, 25}		/* BROWN   */
};

/* The order in this table has no meaning. */
static const struct color_name standard_color_names[] = {
    {"black", BLACK},
    {"red", RED},
    {"green", GREEN},
    {"blue", BLUE},
    {"yellow", YELLOW},
    {"cyan", CYAN},
    {"magenta", MAGENTA},
    {"white", WHITE},
    {"grey", GREY},
    {"gray", GRAY},
    {"orange", ORANGE},
    {"aqua", AQUA},
    {"indigo", INDIGO},
    {"violet", VIOLET},
    {"purple", PURPLE},
    {"brown", BROWN}
};

/*!
   \brief Get number of named colors (RGB triplets)

   \return number of colors
 */
int G_num_standard_colors(void)
{
    return sizeof(standard_colors_rgb) / sizeof(standard_colors_rgb[0]);
}

/*!
   \brief Get RGB triplet of given color

   \param n color index
 */
struct color_rgb G_standard_color_rgb(int n)
{
    return standard_colors_rgb[n];
}

/*!
   \brief Get number of named colors (color names)

   \return number of colors
 */
int G_num_standard_color_names(void)
{
    return sizeof(standard_color_names) / sizeof(standard_color_names[0]);
}

/*!
   \brief Get color name

   \param n color index
 */
const struct color_name *G_standard_color_name(int n)
{
    return &standard_color_names[n];
}

/*! 
   \brief Parse color string and set red,green,blue

   \param str color string
   \param[out] red red value
   \param[out] grn green value
   \param[out] blu blue value

   \return 1 OK
   \return 2 NONE 
   \return 0 on error 
 */
int G_str_to_color(const char *str, int *red, int *grn, int *blu)
{
    char buf[100];
    int num_names = G_num_standard_color_names();
    int i;

    strcpy(buf, str);
    G_chop(buf);

    G_debug(3, "G_str_to_color(): str = '%s'", buf);

    if (G_strcasecmp(buf, "NONE") == 0)
	return 2;

    if (sscanf(buf, "%d%*[,:; ]%d%*[,:; ]%d", red, grn, blu) == 3) {
	if (*red < 0 || *red > 255 ||
	    *grn < 0 || *grn > 255 || *blu < 0 || *blu > 255)
	    return 0;

	return 1;
    }

    int hex;

    if (sscanf(buf, "#%x", &hex) == 1) {
	*red = (hex >> 16) & 0xFF;
	*grn = (hex >> 8) & 0xFF;
	*blu = hex & 0xFF;
	if (*red < 0 || *red > 255 ||
	    *grn < 0 || *grn > 255 || *blu < 0 || *blu > 255)
	    return 0;

	return 1;
    }

    /* Look for this color in the standard (preallocated) colors */
    for (i = 0; i < num_names; i++) {
	const struct color_name *name = &standard_color_names[i];

	if (G_strcasecmp(buf, name->name) == 0) {
	    struct color_rgb rgb = standard_colors_rgb[name->number];

	    *red = (int)rgb.r;
	    *grn = (int)rgb.g;
	    *blu = (int)rgb.b;

	    return 1;
	}
    }

    return 0;
}
