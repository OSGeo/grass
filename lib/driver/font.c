#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include "driver.h"
#include "driverlib.h"

static int font_type = GFONT_STROKE;
static char *encoding;

static void stroke_set(const char *filename)
{
    if (font_init(filename) == 0)
	font_type = GFONT_STROKE;
}

static void freetype_set(const char *filename, int index)
{
    if (font_init_freetype(filename, index) == 0)
	font_type = GFONT_FREETYPE;
}

static void driver_set(const char *name)
{
    (*driver->Set_font)(name);
    font_type = GFONT_DRIVER;
}

int font_get_type(void)
{
    return font_type;
}

const char *font_get_encoding(void)
{
    if (!encoding)
	encoding = G_store("ISO-8859-1");
    return encoding;
}

static void font_list(char ***list, int *count, int verbose)
{
    char **fonts;
    int num_fonts;
    int i;

    for (i = 0; ftcap[i].name; i++)
	;
    num_fonts = i;

    G_debug(2, "font_list: num_fonts=%d", num_fonts);
    fonts = G_malloc(num_fonts * sizeof(const char *));

    for (i = 0; i < num_fonts; i++) {
	struct GFONT_CAP *p = &ftcap[i];

	G_debug(4, "font: %d (%s)", i, p->name);
	if (verbose) {
	    char buf[GPATH_MAX];

	    sprintf(buf, "%s|%s|%d|%s|%d|%s|",
		    p->name, p->longname, p->type,
		    p->path, p->index, p->encoding);

	    fonts[i] = G_store(buf);
	}
	else
	    fonts[i] = G_store(p->name);
    }

    *list = fonts;
    *count = num_fonts;
}

static void free_font_list(char **fonts, int count)
{
    int i;

    for (i = 0; i < count; i++)
	G_free(fonts[i]);

    G_free(fonts);
}

void COM_Set_font(const char *name)
{
    int i;

    if (G_is_absolute_path(name)) {
	if (font_exists(name))
	    freetype_set(name, 0);
	return;
    }

    for (i = 0; ftcap[i].name; i++) {
	struct GFONT_CAP *cap = &ftcap[i];

	if (strcmp(name, cap->name) != 0)
	    continue;

	switch (cap->type) {
	case GFONT_FREETYPE:
	    freetype_set(cap->path, cap->index);
	    COM_Set_encoding(cap->encoding);
	    break;
	case GFONT_STROKE:
	    stroke_set(cap->name);
	    break;
	}
	return;
    }


    if (driver->Font_info && driver->Set_font) {
	char **list = NULL;
	int count = 0;

	(*driver->Font_info)(&list, &count);

	for (i = 0; i < count; i++) {
	    struct GFONT_CAP cap;

	    if (!parse_fontcap_entry(&cap, list[i]))
		continue;

	    if (cap.type != GFONT_DRIVER || strcmp(name, cap.name) != 0)
		continue;

	    driver_set(cap.name);
	    COM_Set_encoding(cap.encoding);
	    break;
	}

	free_font_list(list, count);
	return;
    }

    stroke_set("romans");
}

void COM_Set_encoding(const char *enc)
{
    if (encoding)
	G_free(encoding);

    encoding = G_store(enc);
}

void COM_Font_list(char ***list, int *count)
{
    font_list(list, count, 0);
    if (driver->Font_list)
	(*driver->Font_list)(list, count);
}

void COM_Font_info(char ***list, int *count)
{
    font_list(list, count, 1);
    if (driver->Font_info)
	(*driver->Font_info)(list, count);
}
