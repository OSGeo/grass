#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include "driverlib.h"

static char *filename;
static int font_index;
static char *charset;

int font_init_freetype(const char *name, int index)
{
    if (filename)
	G_free(filename);
    filename = G_store(name);

    font_index = index;

    return 0;
}

int font_init_charset(const char *str)
{
    if (charset)
	G_free(charset);
    charset = G_store(str);
    return 0;
}

const char *font_get_freetype_name(void)
{
    return filename;
}

const char *font_get_charset(void)
{
    if (!charset)
	charset = G_store("ISO-8859-1");
    return charset;
}

int font_get_index(void)
{
    return font_index;
}
