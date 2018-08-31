#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include "driverlib.h"

static char *filename;
static int font_index;

int font_init_freetype(const char *name, int index)
{
    if (filename)
	G_free(filename);
    filename = G_store(name);

    font_index = index;

    return 0;
}

const char *font_get_freetype_name(void)
{
    return filename;
}

int font_get_index(void)
{
    return font_index;
}
