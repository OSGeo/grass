
/****************************************************************************
 *
 * MODULE:       r.reclass
 * AUTHOR(S):    James Westervelt, Michael Shapiro, CERL (original contributors)
 *               Huidae Cho <grass4u gmail.com>, Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>, Jan-Oliver Wagner <jan intevation.de>,
 *               Markus Neteler <neteler itc.it>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006, 2010 by the GRASS Development Team
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
#include <grass/raster.h>
#include <grass/glocale.h>
#include "rule.h"

int main(int argc, char *argv[])
{
    struct Categories cats;
    struct FPRange range;
    DCELL min, max;
    RASTER_MAP_TYPE map_type;
    char buf[1024];
    RULE *rules, *tail;
    int any;
    const char *old_mapset;
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
    G_add_keyword(_("raster"));
    G_add_keyword(_("reclassification"));
    module->label = _("Reclassify raster map based on category values.");
    module->description =
	_("Creates a new raster map whose category values are based "
	  "upon a reclassification of the categories in an existing "
	  "raster map.");

    parm.input = G_define_standard_option(G_OPT_R_INPUT);
    parm.input->description = _("Name of raster map to be reclassified");
    
    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);
    
    parm.rules = G_define_standard_option(G_OPT_F_INPUT);
    parm.rules->key = "rules";
    parm.rules->label = _("File containing reclass rules");
    parm.rules->description = _("'-' for standard input");
    
    parm.title = G_define_option();
    parm.title->key = "title";
    parm.title->required = NO;
    parm.title->type = TYPE_STRING;
    parm.title->description = _("Title for output raster map");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    old_mapset = G_find_raster2(parm.input->answer, "");
    if (old_mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), parm.input->answer);

    if (strcmp(parm.input->answer, parm.output->answer) == 0 &&
	strcmp(old_mapset, G_mapset()) == 0)
	G_fatal_error(_("Input map can NOT be the same as output map"));

    srcfp = stdin;
    if (strcmp(parm.rules->answer, "-") != 0) {
	srcfp = fopen(parm.rules->answer, "r");
	if (!srcfp)
	    G_fatal_error(_("Cannot open rules file <%s>"),
			  parm.rules->answer);
    }
    tty = isatty(fileno(srcfp));

    Rast_init_cats("", &cats);
    map_type = Rast_map_type(parm.input->answer, old_mapset);
    Rast_read_fp_range(parm.input->answer, old_mapset, &range);
    Rast_get_fp_range_min_max(&range, &min, &max);
    rules = tail = NULL;
    any = 0;

    if (tty) {
	fprintf(stderr,
		_("Enter rule(s), \"end\" when done, \"help\" if you need it\n"));
	if (map_type == FCELL_TYPE)
	    fprintf(stderr, _("FCELL: Data range is %.7g to %.7g\n"),
		    (double)min, (double)max);
	else if (map_type == DCELL_TYPE)
	    fprintf(stderr, _("DCELL: Data range is %.15g to %.15g\n"),
		    (double)min, (double)max);
	else
	    fprintf(stderr, _("CELL: Data range is %ld to %ld\n"), (long)min,
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
		G_fatal_error("%s", buf);
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
