#ifndef GRASS_DISPLAY_H
#define GRASS_DISPLAY_H

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/symbol.h>

struct color_rgba
{
    unsigned char r, g, b, a;	/* red, green, blue, and alpha */
};

typedef struct color_rgba RGBA_Color;

/* RGBA color alpha presets */
#define RGBA_COLOR_OPAQUE     255
#define RGBA_COLOR_TRANSPARENT  0
#define RGBA_COLOR_NONE         0

enum clip_mode
{
    M_NONE,
    M_CULL,
    M_CLIP,
};

#include <grass/defs/display.h>

#endif /* GRASS_DISPLAY_H */
