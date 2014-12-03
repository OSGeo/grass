
/****************************************************************************
 *
 * MODULE:       g.dirseps
 * AUTHOR(S):    Paul Kelly
 * PURPOSE:      Copies input string to stdout, changing directory separator
 *               characters as specified by flags.
 *               Used for interoperability between Unix and Windows
 *               pathnames.
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct Flag *tohost, *tograss;
    struct Option *path;
    struct GModule *module;

    G_set_program_name(argv[0]);
    G_no_gisinit();
    G_set_gisrc_mode(G_GISRC_MODE_MEMORY);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("scripts"));
    module->label =
	_("Internal GRASS utility for converting directory separator characters.");
    module->description =
	"Converts any directory separator characters in "
	"the input string to or from native host format, and writes the changed "
	"path to standard output. Useful in scripts for Windows compatibility.";

    tohost = G_define_flag();
    tohost->key = 'h';
    tohost->description =
	"Convert directory separators to native host format";

    tograss = G_define_flag();
    tograss->key = 'g';
    tograss->description =
	"Convert directory separators to GRASS internal format";

    path = G_define_option();
    path->key = "path";
    path->type = TYPE_STRING;
    path->required = NO;
    path->description =
	"Path to be converted (read from stdin if not specified)";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (!tohost->answer && !tograss->answer)
	G_fatal_error("One of flags -%c or -%c must be specified!",
		      tohost->key, tograss->key);

    if (tohost->answer && tograss->answer)
	G_fatal_error("Only one of flags -%c or -%c can be specified!",
		      tohost->key, tograss->key);

    if (path->answer) {
	/* Take input from command-line option */
	char *pathstring = G_store(path->answer);

	if (tohost->answer)
	    G_convert_dirseps_to_host(pathstring);

	if (tograss->answer)
	    G_convert_dirseps_from_host(pathstring);

	puts(pathstring);

    }
    else {
	char inchar;

	while ((inchar = getc(stdin)) != EOF) {
	    /* Read a character at a time from stdin until EOF
	     * and copy to stdout after any conversion */
	    if (tohost->answer) {
		if (inchar == GRASS_DIRSEP)
		    inchar = HOST_DIRSEP;
	    }

	    if (tograss->answer) {
		if (inchar == HOST_DIRSEP)
		    inchar = GRASS_DIRSEP;
	    }

	    putchar(inchar);

	}
    }

    fflush(stdout);

    exit(EXIT_SUCCESS);

}
