
/****************************************************************************
 *
 * MODULE:       r.reclass
 * AUTHOR(S):    James Westervelt, Michael Shapiro, CERL (original contributors)
 *               Huidae Cho <grass4u gmail.com>, Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_nospam yahoo.com>, Jan-Oliver Wagner <jan intevation.de>,
 *               Markus Neteler <neteler itc.it>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "rule.h"

int main(int argc, char *argv[])
{
    struct Categories cats;
    struct FPRange range;
    DCELL min, max;
    int fp;
    char buf[1024];
    RULE *rules, *tail;
    int any;
    char *old_mapset;
    FILE *srcfp;
    int tty;
    struct GModule *module;
    struct
    {
	struct Option *input, *output, *title, *rules;
    } parm;

    /* any interaction must run in a term window */
    G_putenv("GRASS_UI_TERM", "1");

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Creates a new map layer whose category values are based "
	  "upon a reclassification of the categories in an existing "
	  "raster map layer.");

    parm.input = G_define_standard_option(G_OPT_R_INPUT);
    parm.input->description = _("Raster map to be reclassified");

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);

    parm.rules = G_define_option();
    parm.rules->key = "rules";
    parm.rules->type = TYPE_STRING;
    parm.rules->description = _("File containing reclass rules");
    parm.rules->key_desc = "name";
    parm.rules->gisprompt = "old_file,file,input";

    parm.title = G_define_option();
    parm.title->key = "title";
    parm.title->required = NO;
    parm.title->type = TYPE_STRING;
    parm.title->description = _("Title for the resulting raster map");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    old_mapset = G_find_cell2(parm.input->answer, "");
    if (old_mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), parm.input->answer);

    if (G_legal_filename(parm.output->answer) < 0)
	G_fatal_error(_("<%s> is an illegal file name"), parm.output->answer);

    if (strcmp(parm.input->answer, parm.output->answer) == 0 &&
	strcmp(old_mapset, G_mapset()) == 0)
	G_fatal_error(_("Input map can NOT be the same as output map"));

    srcfp = stdin;
    if (parm.rules->answer) {
	srcfp = fopen(parm.rules->answer, "r");
	if (!srcfp)
	    G_fatal_error(_("Cannot open rules file <%s>"),
			  parm.rules->answer);
    }
    tty = isatty(fileno(srcfp));

    G_init_cats(0, "", &cats);
    fp = G_raster_map_is_fp(parm.input->answer, old_mapset);
    G_read_fp_range(parm.input->answer, old_mapset, &range);
    G_get_fp_range_min_max(&range, &min, &max);
    rules = tail = NULL;
    any = 0;

    if (tty) {
	fprintf(stdout,
		_("Enter rule(s), \"end\" when done, \"help\" if you need it\n"));
	if (fp)
	    fprintf(stdout, _("fp: Data range is %.25f to %.25f\n"),
		    (double)min, (double)max);
	else
	    fprintf(stdout, _("Data range is %ld to %ld\n"), (long)min,
		    (long)max);
    }

    while (input(srcfp, tty, buf)) {
	switch (parse(buf, &rules, &tail, &cats)) {
	case -1:
	    if (tty) {
		fprintf(stderr, _("Illegal reclass rule -"));
		fprintf(stderr, _(" ignored\n"));
	    }
	    else {
		strcat(buf, _(" - invalid reclass rule"));
		G_fatal_error(buf);
	    }
	    break;

	case 0:
	    break;

	default:
	    any = 1;
	    break;
	}
    }

    if (!any) {
	if (tty)
	    G_fatal_error(_("No rules specified. Raster map <%s> not created"),
			  parm.output->answer);
	else
	    G_fatal_error(_("No rules specified"));
    }

    reclass(parm.input->answer, old_mapset, parm.output->answer, rules, &cats,
	    parm.title->answer);

    exit(EXIT_SUCCESS);
}
