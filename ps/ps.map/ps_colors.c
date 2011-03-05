/* Functions: get_color_number, get_color_rgb, color_name_is_ok, get_color_name
 **
 ** Author: Paul W. Carlson     March 1992
 */

#include <stdio.h>
#include <string.h>
#include "local_proto.h"
#include "clr.h"

#define	NUM_COLORS	16

static struct
{
    char *name;
    float r, g, b;
} colors[NUM_COLORS] = {
    {
    "white", 1.00, 1.00, 1.00}, {
    "black", 0.00, 0.00, 0.00}, {
    "red", 1.00, 0.00, 0.00}, {
    "green", 0.00, 1.00, 0.00}, {
    "blue", 0.00, 0.00, 1.00}, {
    "yellow", 1.00, 1.00, 0.00}, {
    "magenta", 1.00, 0.00, 1.00}, {
    "cyan", 0.00, 1.00, 1.00}, {
    "aqua", 0.00, 0.75, 0.75}, {
    "grey", 0.75, 0.75, 0.75}, {
    "gray", 0.75, 0.75, 0.75}, {
    "orange", 1.00, 0.50, 0.00}, {
    "brown", 0.75, 0.50, 0.25}, {
    "purple", 0.50, 0.00, 1.00}, {
    "violet", 0.50, 0.00, 1.00}, {
    "indigo", 0.00, 0.50, 1.00}
};

int get_color_number(char *color_name)
{
    int i;

    G_strip(color_name);
    lowercase(color_name);
    for (i = 0; i < NUM_COLORS; i++)
	if (strcmp(color_name, colors[i].name) == 0)
	    return i;
    if (strcmp(color_name, "none") == 0)
	return -999;
    return -1;
}

int get_color_rgb(int color_number, float *r, float *g, float *b)
{
    if (color_number < 0 || color_number >= NUM_COLORS) {
	*r = *g = *b = 0.0;
	return -1;
    }
    *r = colors[color_number].r;
    *g = colors[color_number].g;
    *b = colors[color_number].b;
    return 1;
}

int color_name_is_ok(char *color_name)
{
    int i;

    G_strip(color_name);
    lowercase(color_name);
    for (i = 0; i < NUM_COLORS; i++)
	if (strcmp(color_name, colors[i].name) == 0)
	    return 1;
    return 0;
}

char *get_color_name(int color_number)
{
    if (color_number < 0 || color_number >= NUM_COLORS)
	return (char *)NULL;
    return colors[color_number].name;
}

int set_rgb_color(int color_number)
{
    float r, g, b;

    if (get_color_rgb(color_number, &r, &g, &b) < 0) {
	r = g = b = 0.0;
    }
    fprintf(PS.fp, "%.3f %.3f %.3f C\n", r, g, b);

    return 0;
}

void unset_color(PSCOLOR * pscolor)
{
    pscolor->none = 1;
}

void set_color(PSCOLOR * pscolor, int r, int g, int b)
{
    pscolor->none = 0;

    pscolor->r = r;
    pscolor->g = g;
    pscolor->b = b;

    pscolor->fr = r / 255.0;
    pscolor->fg = g / 255.0;
    pscolor->fb = b / 255.0;
}

void set_color_from_color(PSCOLOR * pscolor, int color)
{
    float r, g, b;

    pscolor->none = 0;

    if (get_color_rgb(color, &r, &g, &b) < 0) {
	r = g = b = 0.0;
    }

    pscolor->r = 255.0 * r;
    pscolor->g = 255.0 * g;
    pscolor->b = 255.0 * b;

    pscolor->fr = r;
    pscolor->fg = g;
    pscolor->fb = b;
}

int set_ps_color(PSCOLOR * pscolor)
{
    fprintf(PS.fp, "%.3f %.3f %.3f C\n", pscolor->fr, pscolor->fg,
	    pscolor->fb);
    return 0;
}

int color_none(PSCOLOR * pscolor)
{
    if (pscolor->none)
	return 1;
    else
	return 0;
}
