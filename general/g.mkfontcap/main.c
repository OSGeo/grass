
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <grass/gis.h>
#include <grass/fontcap.h>
#include <grass/glocale.h>

#include "local_proto.h"

char **searchdirs;
int numsearchdirs;

struct GFONT_CAP *fontcap;
int totalfonts;
int maxfonts;

static const char *standarddirs[] = {
    /* These are the directories that are searched for Freetype-compatible
     * font files by default. They may contain an environment variable 
     * *at the start* of the string, if enclosed in ${xxx} syntax. */
    "/usr/lib/X11/fonts",
    "/usr/share/X11/fonts",
    "/usr/share/fonts",
    "/usr/local/share/fonts",
    "${HOME}/Library/Fonts",
    "/Library/Fonts",
    "/System/Library/Fonts",
    "${WINDIR}/Fonts",
    NULL
};

static void add_search_dir(const char *);
static int compare_fonts(const void *, const void *);

int main(int argc, char *argv[])
{
    struct Flag *tostdout, *overwrite;
    struct Option *extradirs;
    struct GModule *module;

    FILE *outstream;
    char *fontcapfile;
    int i;

    G_set_program_name(argv[0]);
    G_no_gisinit();
    G_set_gisrc_mode(G_GISRC_MODE_MEMORY);

    module = G_define_module();
    G_add_keyword(_("general"));
    module->description =
	_("Generates the font configuration file by scanning various directories "
	  "for fonts.");

    overwrite = G_define_flag();
    overwrite->key = 'o';
    overwrite->description =
	_("Overwrite font configuration file if already existing");

    tostdout = G_define_flag();
    tostdout->key = 's';
    tostdout->description =
	_("Write font configuration file to standard output instead of "
	  "$GISBASE/etc");

    extradirs = G_define_option();
    extradirs->key = "extradirs";
    extradirs->type = TYPE_STRING;
    extradirs->required = NO;
    extradirs->label = _("List of extra directories to scan");
    extradirs->description =
	_("Comma-separated list of extra directories to scan for "
	  "Freetype-compatible fonts as well as the defaults (see documentation)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (!tostdout->answer) {
	const char *gisbase = G_gisbase();
	const char *alt_file = getenv("GRASS_FONT_CAP");

	if (alt_file)
	    fontcapfile = G_store(alt_file);
	else
	    G_asprintf(&fontcapfile, "%s/etc/fontcap", gisbase);

	if (!access(fontcapfile, F_OK)) {	/* File exists? */
	    if (!overwrite->answer)
		G_fatal_error(_("Fontcap file %s already exists; use -%c flag if you "
				"wish to overwrite it"),
			      fontcapfile, overwrite->key);
	}
    }

    searchdirs = NULL;
    numsearchdirs = 0;

    /* Prepare list of directories to search */
    if (extradirs->answer) {
#ifndef HAVE_FT2BUILD_H
	G_warning(_("This GRASS installation was compiled without "
		    "Freetype support, extradirs parameter ignored"));
#endif
	char *str = G_store(extradirs->answer);

	while ((str = strtok(str, ","))) {
	    add_search_dir(str);
	    str = NULL;
	}
    }
    i = -1;
    while (standarddirs[++i])
	add_search_dir(standarddirs[i]);

    totalfonts = maxfonts = 0;
    fontcap = NULL;

    find_stroke_fonts();
    find_freetype_fonts();

    qsort(fontcap, totalfonts, sizeof(struct GFONT_CAP), compare_fonts);

    if (tostdout->answer)
	outstream = stdout;
    else {
	outstream = fopen(fontcapfile, "w");
	if (outstream == NULL)
	    G_fatal_error(_("Cannot open %s for writing: %s"), fontcapfile,
			  strerror(errno));
    }

    for (i = 0; i < totalfonts; i++)
	fprintf(outstream, "%s|%s|%d|%s|%d|%s|\n", fontcap[i].name,
		fontcap[i].longname, fontcap[i].type, fontcap[i].path,
		fontcap[i].index, fontcap[i].encoding);

    fclose(outstream);

    exit(EXIT_SUCCESS);

}

static void add_search_dir(const char *name)
{
    char envvar_name[256];
    char *fullname = NULL;

    if (sscanf(name, "${%255[^}]}", envvar_name) == 1) {
	char *envvar_value = getenv(envvar_name);

	/* N.B. If the envvar isn't set, directory is skipped completely */
	if (envvar_value)
	    G_asprintf(&fullname, "%s%s", envvar_value,
		       (name + strlen(envvar_name) + 3));
    }
    else
	fullname = G_store(name);

    if (fullname) {
	searchdirs = (char **)G_realloc(searchdirs,
					(numsearchdirs + 1) * sizeof(char *));
	searchdirs[numsearchdirs] = fullname;
	G_convert_dirseps_to_host(searchdirs[numsearchdirs]);
	numsearchdirs++;
    }

    return;
}

static int compare_fonts(const void *a, const void *b)
{
    struct GFONT_CAP *aa = (struct GFONT_CAP *)a;
    struct GFONT_CAP *bb = (struct GFONT_CAP *)b;

    /* Sort first by type, then by name */
    if (aa->type != bb->type)
	return (aa->type > bb->type);
    else {
	const char *na = aa->name;
	const char *nb = bb->name;

	return G_strcasecmp(na, nb);
    }
}
