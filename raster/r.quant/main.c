
/****************************************************************************
 *
 * MODULE:       r.quant
 * AUTHOR(S):    Michael Shapiro, Olga Waupotitsch, CERL (original contributors)
 *               Markus Neteler <neteler itc.it>, Roberto Flor <flor itc.it>,
 *               Glynn Clements <glynn gclements.plus.com>, Jachym Cepicky <jachym les-ejk.cz>,
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "global.h"
#include <grass/raster.h>
#include <grass/glocale.h>

struct Quant quant_struct;
CELL old_min, old_max;
DCELL old_dmin, old_dmax;
char **name;		/* input map names */
int noi;

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *input, *basemap, *fprange, *range, *rules;
    struct Flag *trunc, *rnd;
    int truncate;
    int round;
    int i;
    CELL new_min, new_max;
    DCELL new_dmin, new_dmax;
    char *basename;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("quantization"));
    module->description =
	_("Produces the quantization file for a floating-point map.");

    input = G_define_option();
    input->key = "input";
    input->required = YES;
    input->multiple = YES;
    input->type = TYPE_STRING;
    input->gisprompt = "old,cell,raster";
    input->description = _("Raster map(s) to be quantized");

    rules = G_define_standard_option(G_OPT_F_INPUT);
    rules->key = "rules";
    rules->required = NO;
    rules->description = _("Path to rules file (\"-\" to read from stdin)");

    basemap = G_define_option();
    basemap->key = "basemap";
    basemap->type = TYPE_STRING;
    basemap->gisprompt = "old,cell,raster";
    basemap->description = _("Base map to take quant rules from");

    fprange = G_define_option();
    fprange->key = "fprange";
    fprange->key_desc = "dmin,dmax";
    fprange->description = _("Floating point range: dmin,dmax");
    fprange->type = TYPE_STRING;

    range = G_define_option();
    range->key = "range";
    range->key_desc = "min,max";
    range->description = _("Integer range: min,max");
    range->type = TYPE_STRING;

    trunc = G_define_flag();
    trunc->key = 't';
    trunc->description = _("Truncate floating point data");

    rnd = G_define_flag();
    rnd->key = 'r';
    rnd->description = _("Round floating point data");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    truncate = trunc->answer;
    round = rnd->answer;
    basename = basemap->answer;

    i = 0;
    if (truncate)
	i++;
    if (round)
	i++;
    if (basename)
	i++;
    if (fprange->answer || range->answer)
	i++;
    if (rules->answer)
	i++;
    if (i > 1)
	G_fatal_error(_("-%c, -%c, %s=, %s= and %s= are mutually exclusive"),
			trunc->key, rnd->key, rules->key, basemap->key, fprange->key);

    i = 0;
    if (fprange->answer)
	i++;
    if (range->answer)
	i++;
    if (i == 1)
	G_fatal_error(_("%s= and %s= must be used together"),
			fprange->key, range->key);

    for (noi = 0; input->answers[noi]; noi++)
	;
    name = G_malloc(noi * sizeof(char *));
    /* read and check inputs */
    for (noi = 0; input->answers[noi]; noi++) {
	name[noi] = G_store(input->answers[noi]);

	if (Rast_map_type(name[noi], G_mapset()) == CELL_TYPE)
	    G_fatal_error(_("%s is integer map, it can't be quantized"),
			  name[noi]);
    }

    Rast_quant_init(&quant_struct);

    /* now figure out what new quant rules to write */
    if (truncate) {
	G_message(_("Truncating..."));
	Rast_quant_truncate(&quant_struct);
    }

    else if (round) {
	G_message(_("Rounding..."));
	Rast_quant_round(&quant_struct);
    }

    else if (basename)
	/* set the quant to that of basemap */
    {
	if (Rast_map_type(basename, "") == CELL_TYPE)
	    G_fatal_error(_("%s is integer map, it can't be used as basemap"),
			  basename);

	if (Rast_read_quant(basename, "", &quant_struct) <= 0)
	    G_fatal_error(_("unable to read quant rules for basemap <%s>"),
			  basename);
    }

    else if (fprange->answer)
    {
	if (sscanf(fprange->answer, "%lf,%lf", &new_dmin, &new_dmax) != 2)
	    G_fatal_error(_("invalid value for %s= <%s>"),
			    fprange->key, fprange->answer);
	if (sscanf(range->answer, "%d,%d", &new_min, &new_max) != 2)
	    G_fatal_error(_("invalid value for %s= <%s>"),
			    range->key, range->answer);
	G_message(_("Setting quant rules for input map(s) to (%f,%f) -> (%d,%d)"),
		  new_dmin, new_dmax, new_min, new_max);
	Rast_quant_add_rule(&quant_struct, new_dmin, new_dmax, new_min, new_max);
    }

    else if (rules->answer) {
	if (!read_rules(rules->answer))
	    G_fatal_error("No rules specified");
    }

    else {			/* ask user for quant rules */
	if (!read_rules("-")) {
	    if (isatty(0))
		G_message(_("No rules specified. Quant table(s) not changed."));
	    else
		G_fatal_error(_("No rules specified"));
	}
    }				/* use rules */

    for (i = 0; i < noi; i++) {
	Rast_write_quant(name[i], G_mapset(), &quant_struct);
	G_message(_("New quant table created for %s"), name[i]);
    }

    exit(EXIT_SUCCESS);
}
