/* Takes a color name in ascii, returns the color number for that color.
 *    returns 0 if color is not known.
 */

#include <string.h>

#include <grass/display.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "driver.h"

static struct color_rgb *colors;
static int ncolors;
static int nalloc;

/*!
 * \brief color name and suggested number to actual number
 *
 * Takes a color <b>name</b> or <b>red:green:blue</b> code in ascii
 * and a <b>suggested color index</b>.
 * If the color is a standard preallocated color it returns the color number for that color.
 * Otherwise (if the color is not standard) it sets the color of the supplied index to 
 * the specified color.
 * Returns -1 if color is not known and 0 if the color is none.
 *
 *  \param name_or_code
 *  \param suggested_color_index
 *  \return int
 */

static int translate_or_add_color(const char *str)
{
    int num_names = G_num_standard_color_names();
    int index;
    int red, grn, blu;
    int i, ret;
    char lowerstr[MAX_COLOR_LEN];

    /* Make the color string lowercase for display colors */
    strcpy(lowerstr, str);
    G_chop(lowerstr);
    G_tolcase(lowerstr);

    for (i = 0; i < num_names; i++) {
	const struct color_name *name = G_standard_color_name(i);

	if (G_strcasecmp(str, name->name) == 0)
	    return name->number;
    }

    if (!nalloc) {
	ncolors = G_num_standard_colors();
	nalloc = 2 * ncolors;
	colors = G_malloc(nalloc * sizeof(struct color_rgb));
	for (i = 0; i < ncolors; i++)
	    colors[i] = G_standard_color_rgb(i);
    }

    ret = G_str_to_color(str, &red, &grn, &blu);

    /* None color */
    if (ret == 2)
	return 0;

    if (ret != 1)
	return -1;

    for (i = 1; i < ncolors; i++)
	if (colors[i].r == red && colors[i].g == grn && colors[i].b == blu)
	    return i;

    if (ncolors >= nalloc) {
	nalloc *= 2;
	colors = G_realloc(colors, nalloc * sizeof(struct color_rgb));
    }

    index = ncolors++;

    colors[index].r = red;
    colors[index].g = grn;
    colors[index].b = blu;

    return index;
}

/*!
 * \brief color option text to usable color number
 *
 * Converts or looks up the color provided in the string.
 * Returns a color number usable by D_use_color.
 * If the color does not exist exits with a fatal error and message.
 * If the color is none and none_acceptable is not true exits with
 * a fatal error and message.
 *
 *  \param name_or_code
 *  \param none_acceptable
 *  \return int
 */

int D_parse_color(const char *str, int none_acceptable)
{
    int color;

    color = translate_or_add_color(str);
    if (color == -1)
	G_fatal_error(_("[%s]: No such color"), str);
    if (color == 0 && !none_acceptable)
	G_fatal_error(_("[%s]: No such color"), str);
    return color;
}


/*!
 * \brief color name to number
 *
 * Takes a
 * color <b>name</b> in ascii and returns the color number for that color.
 * Returns 0 if color is not known. The color number returned is for lines and
 * text, not raster graphics.
 *
 *  \param name
 *  \return int
 */

int D_translate_color(const char *str)
{
    return D_parse_color(str, 0);
}


/*!
 * \brief draw with a color from D_parse_color
 *
 * Calls R_color or R_standard_color to use the color provided by 
 * D_parse_color. Returns 1 if color can be used to draw (is
 * good and isn't none), 0 otherwise.
 *
 *  \param color
 *  \return int
 */

int D_use_color(int color)
{
    if (color <= 0)
	return 0;

    if (color < G_num_standard_colors()) {
	COM_Standard_color(color);
	return 1;
    }

    if (color < ncolors) {
	const struct color_rgb *c = &colors[color];

	D_RGB_color(c->r, c->g, c->b);
	return 1;
    }

    return 0;
}


/*!
 * \brief get RGB values from color number
 *
 * Translates the color number provided by D_parse_color
 * into 0-255 RGB values.
 *
 * Returns 1 if color can be used to draw (is good and
 * isn't 'none'), 0 otherwise.
 *
 *  \param color_number
 *  \param red
 *  \param green
 *  \param blue
 *
 *  \return int
 */
int D_color_number_to_RGB(int color, int *r, int *g, int *b)
{
    const struct color_rgb *c;

    if (color <= 0)
	return 0;

    if (color < G_num_standard_colors()) {
	struct color_rgb col = G_standard_color_rgb(color);

	if (r)
	    *r = col.r;
	if (g)
	    *g = col.g;
	if (b)
	    *b = col.b;

	return 1;
    }

    if (color >= ncolors)
	return 0;

    c = &colors[color];
    if (r)
	*r = c->r;
    if (g)
	*g = c->g;
    if (b)
	*b = c->b;

    return 1;
}

void D_RGB_color(int red, int grn, int blu)
{
    COM_Color_RGB(red, grn, blu);
}

