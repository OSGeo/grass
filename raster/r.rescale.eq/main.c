
/****************************************************************************
 *
 * MODULE:       r.rescale.eq
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, Bernhard Reiter <bernhard intevation.de>,
 *               Glynn Clements <glynn gclements.plus.com>, Jachym Cepicky <jachym les-ejk.cz>
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
#include <grass/raster.h>
#include "local_proto.h"
#include <grass/glocale.h>

static FILE *fp;
static void reclass(CELL, CELL, CELL);

int main(int argc, char *argv[])
{
    char input[GNAME_MAX+8];
    char output[GNAME_MAX+8];
    char title[GPATH_MAX];
    char rules[GNAME_MAX+8];
    const char *args[6];
    struct Popen child;
    CELL old_min, old_max;
    CELL new_min, new_max;
    long cat;
    struct Cell_stats statf;
    char *old_name;
    char *new_name;
    struct
    {
	struct Option *input, *from, *output, *to, *title;
    } parm;

    struct GModule *module;

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("rescale"));
    module->description =
	_("Rescales histogram equalized the range of category "
	  "values in a raster map layer.");

    /* Define the different options */

    parm.input = G_define_option();
    parm.input->key = "input";
    parm.input->type = TYPE_STRING;
    parm.input->required = YES;
    parm.input->gisprompt = "old,cell,raster";
    parm.input->description = _("The name of the raster map to be rescaled");

    parm.from = G_define_option();
    parm.from->key = "from";
    parm.from->key_desc = "min,max";
    parm.from->type = TYPE_INTEGER;
    parm.from->required = NO;
    parm.from->description =
	_("The input data range to be rescaled (default: full range of input map)");

    parm.output = G_define_option();
    parm.output->key = "output";
    parm.output->type = TYPE_STRING;
    parm.output->required = YES;
    parm.output->gisprompt = "new,cell,raster";
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

    get_stats(old_name, &statf);
    if (parm.from->answer) {
	sscanf(parm.from->answers[0], "%d", &old_min);
	sscanf(parm.from->answers[1], "%d", &old_max);
    }
    else
	get_range(&statf, &old_min, &old_max, 0);

    if (old_min > old_max) {
	cat = old_min;		/* swap */
	old_min = old_max;
	old_max = cat;
    }

    sscanf(parm.to->answers[0], "%d", &new_min);
    sscanf(parm.to->answers[1], "%d", &new_max);
    if (new_min > new_max) {
	cat = new_min;		/* swap */
	new_min = new_max;
	new_max = cat;
    }
    G_message(_("Rescale %s[%d,%d] to %s[%d,%d]"),
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

    Rast_cell_stats_histo_eq(&statf,
			     old_min, old_max,
			     new_min, new_max,
			     0, reclass);
    G_popen_close(&child);

    return EXIT_SUCCESS;
}

static void reclass(CELL cat1, CELL cat2, CELL value)
{
    fprintf(fp, "%ld thru %ld = %ld %ld",
	    (long)cat1, (long)cat2, (long)value, (long)cat1);
    if (cat1 != cat2)
	fprintf(fp, " thru %ld", (long)cat2);
    fprintf(fp, "\n");
}
