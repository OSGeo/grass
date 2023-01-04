/*
 * from lib/driver/parse_ftcap.c
 * These functions are copied from the diplay driver lib,
 * so that we don't need to have an active display driver open,
 * to run this module.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "labels.h"

static int font_exists(const char *name);

static int font_exists(const char *name)
{
    FILE *fp;

    fp = fopen(name, "r");
    if (!fp)
        return 0;

    fclose(fp);
    return 1;
}

void free_freetypecap(struct GFONT_CAP *ftcap)
{
    if (ftcap == NULL)
        return;
    G_free(ftcap->name);
    G_free(ftcap->longname);
    G_free(ftcap->path);
    G_free(ftcap->encoding);
    G_free(ftcap);

    return;
}

struct GFONT_CAP *find_font_from_freetypecap(const char *font)
{
    char *capfile, file[GPATH_MAX];
    char buf[GPATH_MAX];
    FILE *fp;
    int fonts_count = 0;
    struct GFONT_CAP *font_cap = NULL;

    fp = NULL;
    if ((capfile = getenv("GRASS_FONT_CAP"))) {
        if ((fp = fopen(capfile, "r")) == NULL)
            G_warning(
                _("%s: Unable to read font definition file; use the default"),
                capfile);
    }
    if (fp == NULL) {
        sprintf(file, "%s/etc/fontcap", G_gisbase());
        if ((fp = fopen(file, "r")) == NULL)
            G_warning(_("%s: No font definition file"), file);
    }

    if (fp != NULL) {
        while (fgets(buf, sizeof(buf), fp) && !feof(fp)) {
            char name[GNAME_MAX], longname[GNAME_MAX], path[GPATH_MAX],
                encoding[128];
            int type, index;
            char *p;

            p = strchr(buf, '#');
            if (p)
                *p = 0;

            if (sscanf(buf, "%[^|]|%[^|]|%d|%[^|]|%d|%[^|]|", name, longname,
                       &type, path, &index, encoding) != 6)
                continue;
            if (strcmp(name, font) != 0 && strcmp(longname, font) != 0)
                continue;
            if (!font_exists(path))
                continue;

            font_cap = (struct GFONT_CAP *)G_malloc(sizeof(struct GFONT_CAP));

            font_cap[fonts_count].name = G_store(name);
            font_cap[fonts_count].longname = G_store(longname);
            font_cap[fonts_count].type = type;
            font_cap[fonts_count].path = G_store(path);
            font_cap[fonts_count].index = index;
            font_cap[fonts_count].encoding = G_store(encoding);
        }
        fclose(fp);
    }

    return font_cap;
}
