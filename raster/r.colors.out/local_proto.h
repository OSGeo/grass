#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/G3d.h>
#include <grass/raster.h>
#include <grass/glocale.h>

void write_colors(struct Colors *, struct FPRange*, const char *, int);