#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/freetypecap.h>
#include "driverlib.h"

int font_exists(const char *name)
{
    FILE *fp;

    fp = fopen(name, "r");
    if (!fp)
	return 0;

    fclose(fp);
    return 1;
}

struct GFONT_CAP *parse_freetypecap(void)
{
    char *capfile, file[GPATH_MAX];
    char buf[GPATH_MAX];
    FILE *fp;
    int fonts_count = 0;
    struct GFONT_CAP *fonts = NULL;

    fp = NULL;
    if ((capfile = getenv("GRASS_FONT_CAP"))) {
	if ((fp = fopen(capfile, "r")) == NULL)
	    G_warning(_("%s: Unable to read font definition file; use the default"),
		      capfile);
    }
    if (fp == NULL) {
	sprintf(file, "%s/etc/fontcap", G_gisbase());
	if ((fp = fopen(file, "r")) == NULL)
	    G_warning(_("%s: No font definition file"), file);
    }

    if (fp != NULL) {
	while (fgets(buf, sizeof(buf), fp) && !feof(fp)) {
	    char name[GNAME_MAX], longname[GNAME_MAX],
		path[GPATH_MAX], encoding[128];
	    int type, index;
	    char *p;

	    p = strchr(buf, '#');
	    if (p)
		*p = 0;

	    if (sscanf(buf, "%[^|]|%[^|]|%d|%[^|]|%d|%[^|]|",
		       name, longname, &type, path, &index, encoding)
		!= 6)
		continue;

	    if (!font_exists(path))
		continue;

	    fonts = (struct GFONT_CAP *)G_realloc(fonts,
						  (fonts_count +
						   1) *
						  sizeof(struct GFONT_CAP));

	    fonts[fonts_count].name = G_store(name);
	    fonts[fonts_count].longname = G_store(longname);
	    fonts[fonts_count].type = type;
	    fonts[fonts_count].path = G_store(path);
	    fonts[fonts_count].index = index;
	    fonts[fonts_count].encoding = G_store(encoding);

	    fonts_count++;
	}
	fclose(fp);
    }

    fonts = (struct GFONT_CAP *)G_realloc(fonts, (fonts_count + 1) *
					  sizeof(struct GFONT_CAP));
    fonts[fonts_count].name = NULL;
    fonts[fonts_count].path = NULL;

    return fonts;
}

void free_freetypecap(struct GFONT_CAP *ftcap)
{
    int i;

    if (ftcap == NULL)
	return;

    for (i = 0; ftcap[i].name; i++) {
	G_free(ftcap[i].name);
	G_free(ftcap[i].longname);
	G_free(ftcap[i].path);
	G_free(ftcap[i].encoding);
    }

    G_free(ftcap);

    return;
}
