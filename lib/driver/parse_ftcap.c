/*!
  \file lib/driver/parse_ftcap.c

  \brief Display Driver - fontcaps

  (C) 2006-2011 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.

  \author Glynn Clements <glynn gclements.plus.com> (original contributor)
  \author Huidae Cho <grass4u gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/fontcap.h>
#include "driverlib.h"

/*!
  \brief Check if font exists
*/
int font_exists(const char *name)
{
    return access(name, R_OK) >= 0;
}

/*!
  \brief Parse fontcap entry

  \param e pointer to GFONT_CAP struct
  \param str ?

  \return 1 on success
  \return 0 on failure
*/
int parse_fontcap_entry(struct GFONT_CAP *e, const char *str)
{
    char name[GNAME_MAX], longname[GNAME_MAX], path[GPATH_MAX], encoding[128];
    int type, index;

    if (sscanf(str, "%[^|]|%[^|]|%d|%[^|]|%d|%[^|]|",
	       name, longname, &type, path, &index, encoding) == 6) {
        if (!font_exists(path))
	    return 0;
    }
    /* GFONT_DRIVER type fonts do not have path. */
    else if (sscanf(str, "%[^|]|%[^|]|%d||%d|%[^|]|",
	       name, longname, &type, &index, encoding) == 5)
	path[0] = '\0';
    else
	return 0;

    e->name = G_store(name);
    e->longname = G_store(longname);
    e->type = type;
    e->path = G_store(path);
    e->index = index;
    e->encoding = G_store(encoding);

    return 1;
}

/*!
  \brief Parse fontcaps

  \return pointer to GFONT_CAP structure
*/
struct GFONT_CAP *parse_fontcap(void)
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
	    struct GFONT_CAP cap;
	    char *p;

	    p = strchr(buf, '#');
	    if (p)
		*p = 0;

	    if (!parse_fontcap_entry(&cap, buf))
		continue;

	    fonts = G_realloc(fonts, (fonts_count + 1) * sizeof(struct GFONT_CAP));
	    fonts[fonts_count++] = cap;
	}

	fclose(fp);
    }

    fonts = G_realloc(fonts, (fonts_count + 1) * sizeof(struct GFONT_CAP));
    fonts[fonts_count].name = NULL;
    fonts[fonts_count].path = NULL;

    return fonts;
}

/*!
  \brief Free allocated GFONT_CAP structure

  \param ftcap pointer to GFONT_CAP to be freed
*/
void free_fontcap(struct GFONT_CAP *ftcap)
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
}
