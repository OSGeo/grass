#ifndef GRASS_COLORSDEFS_H
#define GRASS_COLORSDEFS_H

int G_num_standard_colors(void);
struct color_rgb G_standard_color_rgb(int);
int G_num_standard_color_names(void);
const struct color_name *G_standard_color_name(int);
int G_str_to_color(const char *, int *, int *, int *);

#endif
