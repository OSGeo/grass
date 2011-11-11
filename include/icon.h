#ifndef GRASS_ICON_H
#define GRASS_ICON_H
#ifndef _STDIO_H
#include <stdio.h>
#endif

#define ICON struct _icon
struct _icon
{
    int nrows;
    int ncols;
    int xref;
    int yref;
    char **map;
};

#include <grass/defs/icon.h>

#endif
