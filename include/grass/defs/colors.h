#ifndef GRASS_COLORSDEFS_H
#define GRASS_COLORSDEFS_H

#include <grass/gis.h>

int G_num_standard_colors(void);
struct color_rgb G_standard_color_rgb(int);
int G_num_standard_color_names(void);
const struct color_name *G_standard_color_name(int);
int G_str_to_color(const char *, int *, int *, int *);
void G_rgb_to_hsv(int, int, int, float *, float *, float *);
void G_color_to_str(int, int, int, ColorFormat, char *);
ColorFormat G_option_to_color_format(const struct Option *);

#endif
