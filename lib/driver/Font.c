#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include "driver.h"
#include "driverlib.h"

static int font_type = GFONT_STROKE;

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

void COM_Font_get(const char *name)
{
    if (G_is_absolute_path(name)) {
	if (!font_exists(name))
	    return;

	freetype_set(name, 0);
    }
    else {
	int i;

	/* check if freetype font is available in freetypecap */
	for (i = 0; ftcap[i].name; i++)
	    if (strcmp(name, ftcap[i].name) == 0) {
		switch (ftcap[i].type) {
		case GFONT_FREETYPE:
		    freetype_set(ftcap[i].path, ftcap[i].index);
		    font_init_charset(ftcap[i].encoding);
		    break;
		case GFONT_STROKE:
		    stroke_set(ftcap[i].name);
		    break;
		}
		return;
	    }

	stroke_set("romans");
    }
}

void COM_Font_init_charset(const char *charset)
{
    font_init_charset(charset);
}

int font_is_freetype(void)
{
    return font_type == GFONT_FREETYPE;
}

static void font_list(char ***list, int *count, int verbose)
{
    char **fonts;
    int num_fonts;
    int i;

    for (i = 0; ftcap[i].name; i++) ;

    num_fonts = i;

    fonts = G_malloc(num_fonts * sizeof(const char *));

    for (i = 0; i < num_fonts; i++) {
	struct GFONT_CAP *p = &ftcap[i];

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

void COM_Font_list(char ***list, int *count)
{
    font_list(list, count, 0);
}

void COM_Font_info(char ***list, int *count)
{
    font_list(list, count, 1);
}

void free_font_list(char **fonts, int count)
{
    int i;

    for (i = 0; i < count; i++)
	G_free(fonts[i]);
    G_free(fonts);
}
