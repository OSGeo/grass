
/****************************************************************************
 *
 * MODULE:       r.rescale
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Jachym Cepicky <jachym les-ejk.cz>, Jan-Oliver Wagner <jan intevation.de>
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
#include <string.h>
#include <grass/gis.h>
#include "local_proto.h"
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    char input[GNAME_MAX+8];
    char output[GNAME_MAX+8];
    char rules[GNAME_MAX+8];
    char title[GPATH_MAX];
    const char *args[6];
    struct Popen child;
    FILE *fp;
    long old_min, old_max;
    long new_min, new_max;
    long new_delta, old_delta;
    long value, first, prev;
    long cat;
    float divisor;
    char *old_name;
    char *new_name;
    struct GModule *module;
    struct
    {
	struct Option *input, *from, *output, *to, *title;
    } parm;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("rescale"));
    module->description =
	_("Rescales the range of category values " "in a raster map layer.");

    /* Define the different options */

    parm.input = G_define_standard_option(G_OPT_R_INPUT);
    parm.input->description = _("The name of the raster map to be rescaled");

    parm.from = G_define_option();
    parm.from->key = "from";
    parm.from->key_desc = "min,max";
    parm.from->type = TYPE_INTEGER;
    parm.from->required = NO;
    parm.from->description =
	_("The input data range to be rescaled (default: full range of input map)");

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.output->description = _("The resulting raster map name");

    parm.to = G_define_option();
    parm.to->key = "to";
    parm.to->key_desc = "min,max";
    parm.to->type = TYPE_INTEGER;
    parm.to->required = YES;
    parm.to->description = _("The output data range");

    parm.title = G_define_option();
    parm.title->key = "title";
    parm.title->key_desc = "phrase";
    parm.title->type = TYPE_STRING;
    parm.title->required = NO;
    parm.title->description = _("Title for new raster map");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    old_name = parm.input->answer;
    new_name = parm.output->answer;

    if (parm.from->answer) {
	sscanf(parm.from->answers[0], "%ld", &old_min);
	sscanf(parm.from->answers[1], "%ld", &old_max);

    }
    else
	get_range(old_name, &old_min, &old_max);
    if (old_min > old_max) {
	value = old_min;	/* swap */
	old_min = old_max;
	old_max = value;
    }

    sscanf(parm.to->answers[0], "%ld", &new_min);
    sscanf(parm.to->answers[1], "%ld", &new_max);
    if (new_min > new_max) {
	value = new_min;	/* swap */
	new_min = new_max;
	new_max = value;
    }

    G_message(_("Rescale %s[%ld,%ld] to %s[%ld,%ld]"),
	      old_name, old_min, old_max, new_name, new_min, new_max);

    sprintf(input, "input=%s", old_name);
    sprintf(output, "output=%s", new_name);

    if (parm.title->answer)
	sprintf(title, "title=%s", parm.title->answer);
    else
	sprintf(title, "title=rescale of %s", old_name);

    sprintf(rules, "rules=-");

    args[0] = "r.reclass";
    args[1] = input;
    args[2] = output;
    args[3] = title;
    args[4] = rules;
    args[5] = NULL;

    fp = G_popen_write(&child, "r.reclass", args);

    old_delta = old_max - old_min;
    new_delta = new_max - new_min;
    divisor = (float)new_delta / (float)old_delta;

    prev = new_min;
    first = old_min;
    for (cat = old_min; cat <= old_max; cat++) {
	value = (int)(divisor * (cat - old_min) + new_min + .5);
	if (value != prev) {
	    fprintf(fp, "%ld thru %ld = %ld %ld", first, cat - 1, prev,
		    first);
	    if (cat - 1 != first)
		fprintf(fp, " thru %ld", cat - 1);
	    fprintf(fp, "\n");
	    prev = value;
	    first = cat;
	}
    }
    fprintf(fp, "%ld thru %ld = %ld %ld", first, cat - 1, prev, first);
    if (cat - 1 != first)
	fprintf(fp, " thru %ld", cat - 1);
    fprintf(fp, "\n");

    G_popen_close(&child);

    return EXIT_SUCCESS;
}
