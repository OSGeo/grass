
/****************************************************************************
 *
 * MODULE:       g.mkfontcap
 * AUTHOR(S):    Paul Kelly
 * PURPOSE:      Generates the font configuration file by scanning various
 *               directories for GRASS stroke and Freetype-compatible fonts.
 *              
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <grass/gis.h>
#include <grass/fontcap.h>

#include "local_proto.h"

struct font_desc
{

    char *filename;   /**< Filename in fonts directory */

    char *description;/**< Descriptive name of font contained in this file */
};

static struct font_desc *font_descriptions = NULL;
static int num_descriptions = 0;

static int load_font_descriptions(const char *);
static void free_font_descriptions(void);
static const char *get_desc(const char *);

/**
 * \brief Find Stroke fonts and store them in a global GFONT_CAP struct
 * 
 * The directory $GISBASE/fonts is listed to find all stroke fonts (i.e.
 * files with a .hmp extension).
 * Information on each font is stored in the global GFONT_CAP struct, 
 * fontcap, to be used by the main program.
 **/

void find_stroke_fonts(void)
{
    char *dirpath, *fonttable;
    char **dirlisting;
    int numfiles, i;

    G_asprintf(&dirpath, "%s/fonts", G_gisbase());

    dirlisting = G_ls2(dirpath, &numfiles);

    G_asprintf(&fonttable, "%s/fonts.table", dirpath);
    if (access(fonttable, R_OK) == 0)
	load_font_descriptions(fonttable);

    for (i = 0; i < numfiles; i++) {
	if (!strstr(dirlisting[i], ".hmp"))
	    continue;

	if (totalfonts >= maxfonts) {
	    maxfonts += 20;
	    fontcap = G_realloc(fontcap, maxfonts * sizeof(struct GFONT_CAP));
	}

	/* Path */
	G_asprintf(&fontcap[totalfonts].path, "%s%c%s", dirpath,
		   HOST_DIRSEP, dirlisting[i]);
	G_convert_dirseps_to_host(fontcap[totalfonts].path);
	/* Description & Name */
	fontcap[totalfonts].longname = G_store(get_desc(dirlisting[i]));
	*(strstr(dirlisting[i], ".hmp")) = '\0';
	fontcap[totalfonts].name = G_store(dirlisting[i]);
	/* Font Type */
	fontcap[totalfonts].type = GFONT_STROKE;
	/* These two probably not relevant */
	fontcap[totalfonts].index = 0;
	fontcap[totalfonts].encoding = G_store("utf-8");
	totalfonts++;

	G_free(dirlisting[i]);
    }
    G_free(dirlisting);

    if (font_descriptions)
	free_font_descriptions();

    return;

}

/**
 * \brief Loads description file for stroke fonts into memory
 *
 * Parses the font description file and loads an array of filenames
 * and corresponding descriptions pointed at by the static variable
 * font_descriptions. After calling, num_descriptions will contain
 * the number of font descriptions that were loaded.
 * 
 * \param descfile Filename to be loaded
 *
 * \return 1 if at least one font description was loaded; 0 otherwise
 **/

static int load_font_descriptions(const char *descfile)
{
    size_t memsize = 0;
    FILE *fp;
    char buff[500];

    fp = fopen(descfile, "r");
    if (fp == NULL) {
	G_warning("Unable to open font description file %s for reading: %s",
		  descfile, strerror(errno));
	return 0;
    }

    while (G_getl2(buff, sizeof(buff), fp)) {
	char name[100], description[256];

	if (buff[0] == '#')
	    continue;

	if (sscanf(buff, "%99[^|]|%255[^\n]", name, description) != 2)
	    continue;

	if (num_descriptions >= memsize) {
	    memsize += 20;
	    font_descriptions = G_realloc(font_descriptions,
					  memsize * sizeof(struct font_desc));
	}

	font_descriptions[num_descriptions].filename = G_store(name);
	font_descriptions[num_descriptions].description =
	    G_store(description);
	num_descriptions++;
    }

    fclose(fp);

    return (num_descriptions > 0);
}

/**
 * \brief Returns the descriptive name corresponding to a stroke font
 *
 * Searches through the static font_descriptions name for a descriptive
 * name matching the filename passed to the function. If a match is found,
 * the descriptive name is returned; otherwise the filename that was 
 * originally passed is returned.
 * 
 * \param filename Filename of stroke font
 * 
 * \return Const string containing descriptive name for the font
 **/

static const char *get_desc(const char *filename)
{
    int i;

    for (i = 0; i < num_descriptions; i++)
	if (G_strcasecmp(filename, font_descriptions[i].filename) == 0)
	    return font_descriptions[i].description;

    /* If there was no font descriptions file, or the filename wasn't found
     * in it, we'll end up here and simply return the filename for the
     * description */

    return filename;
}

/**
 * \brief Frees the memory used by the table of font descriptive names
 **/

static void free_font_descriptions(void)
{
    int i;

    for (i = 0; i < num_descriptions; i++) {
	G_free(font_descriptions[i].filename);
	G_free(font_descriptions[i].description);
    }

    return;
}
