
/****************************************************************************
 *
 * MODULE:       g.parser
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com> (original contributor)
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Cedric Shock <cedricgrass shockfamily.net>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Paul Kelly <paul-grass stjohnspoint.co.uk>, 
 *               Radim Blazek <radim.blazek gmail.com>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2001-2007, 2010-2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <grass/glocale.h>

#include "proto.h"

int translate_output;

int main(int argc, char *argv[])
{
    struct context ctx;
    const char *filename;
    int standard_output;
    int separator_nul;
    
    ctx.module = NULL;
    ctx.option = NULL;
    ctx.flag = NULL;
    ctx.first_option = NULL;
    ctx.first_flag = NULL;
    ctx.state = S_TOPLEVEL;

    standard_output = translate_output = separator_nul = FALSE;

    /* Detect request to get strings to translate from a file */
    /* It comes BEFORE the filename to completely avoid confusion with parser.c behaviours */
    if (argc >= 2 && (strcmp(argv[1], "-t") == 0)) {
	/* Turn on translation output */
	translate_output = TRUE;
	argv++, argc--;
    }

    if (argc >= 2 && (strcmp(argv[1], "-s") == 0)) {
	/* write to stdout rather than re-invoking */
	standard_output = TRUE;
	argv++, argc--;
    }

    if (argc >= 2 && (strcmp(argv[1], "-n") == 0)) {
	/* write to stdout with NUL as separator */
	standard_output = TRUE;
	separator_nul = TRUE;
	argv++, argc--;
    }

    if ((argc < 2) || ((strcmp(argv[1], "help") == 0) ||
		       (strcmp(argv[1], "-help") == 0) ||
		       (strcmp(argv[1], "--help") == 0))) {
	fprintf(stderr, "%s: %s [-t] [-s] <filename> [<argument> ...]\n",
		_("Usage"), argv[0]);
	exit(EXIT_FAILURE);
    }

    filename = argv[1];
    argv++, argc--;
    G_debug(2, "filename = %s", filename);

    ctx.fp = fopen(filename, "r");
    if (!ctx.fp) {
	perror(_("Unable to open script file"));
	exit(EXIT_FAILURE);
    }

    G_gisinit((char *)filename);

    for (ctx.line = 1;; ctx.line++) {
	char buff[4096];
	char *cmd, *arg;

	if (!fgets(buff, sizeof(buff), ctx.fp))
	    break;

	arg = strchr(buff, '\n');
	if (!arg) {
	    fprintf(stderr, _("Line too long or missing newline at line %d\n"),
		    ctx.line);
	    exit(EXIT_FAILURE);
	}
	*arg = '\0';

	if (buff[0] != '#' || buff[1] != '%')
	    continue;

	cmd = buff + 2;
	G_chop(cmd);

	arg = strchr(cmd, ':');

	if (arg) {
	    *(arg++) = '\0';
	    G_strip(cmd);
	    G_strip(arg);
	}

	switch (ctx.state) {
	case S_TOPLEVEL:
	    parse_toplevel(&ctx, cmd);
	    break;
	case S_MODULE:
	    parse_module(&ctx, cmd, arg);
	    break;
	case S_FLAG:
	    parse_flag(&ctx, cmd, arg);
	    break;
	case S_OPTION:
	    parse_option(&ctx, cmd, arg);
	    break;
	}
    }

    if (fclose(ctx.fp) != 0) {
	perror(_("Error closing script file"));
	exit(EXIT_FAILURE);
    }

    /* Stop here successfully if all that was desired was output of text to translate */
    /* Continuing from here would get argc and argv all wrong in G_parser. */
    if (translate_output)
	exit(EXIT_SUCCESS);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    return standard_output
	? print_options(&ctx, separator_nul ? '\0' : '\n')
	: reinvoke_script(&ctx, filename);
}
