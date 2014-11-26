
/****************************************************************************
 *
 * MODULE:       r.describe
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Prints terse list of category values found in a raster 
 *               map layer.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include "local_proto.h"
#include <grass/glocale.h>


int main(int argc, char *argv[])
{
    int as_int;
    int compact;
    int range;
    int windowed;
    int nsteps;
    char *no_data_str;
    struct GModule *module;
    struct
    {
	struct Flag *one;
	struct Flag *r;
	struct Flag *d;
	struct Flag *i;
	struct Flag *n;
    } flag;
    struct
    {
	struct Option *map;
	struct Option *nv;
	struct Option *nsteps;
    } option;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("metadata"));
    module->description =
	_("Prints terse list of category values found in a raster map layer.");

    /* define different options */
    option.map = G_define_standard_option(G_OPT_R_MAP);

    option.nv = G_define_standard_option(G_OPT_M_NULL_VALUE);
    option.nv->answer = "*";

    option.nsteps = G_define_option();
    option.nsteps->key = "nsteps";
    option.nsteps->type = TYPE_INTEGER;
    option.nsteps->required = NO;
    option.nsteps->multiple = NO;
    option.nsteps->answer = "255";
    option.nsteps->description = _("Number of quantization steps");

    /*define the different flags */

    flag.one = G_define_flag();
    flag.one->key = '1';
    flag.one->description = _("Print the output one value per line");

    flag.r = G_define_flag();
    flag.r->key = 'r';
    flag.r->description = _("Only print the range of the data");

    flag.n = G_define_flag();
    flag.n->key = 'n';
    flag.n->description = _("Suppress reporting of any NULLs");

    flag.d = G_define_flag();
    flag.d->key = 'd';
    flag.d->description = _("Use the current region");

    flag.i = G_define_flag();
    flag.i->key = 'i';
    flag.i->description = _("Read floating-point map as integer");

    if (0 > G_parser(argc, argv))
	exit(EXIT_FAILURE);

    compact = (!flag.one->answer);
    range = flag.r->answer;
    windowed = flag.d->answer;
    as_int = flag.i->answer;
    no_data_str = option.nv->answer;

    if (sscanf(option.nsteps->answer, "%d", &nsteps) != 1 || nsteps < 1)
	G_fatal_error(_("%s = %s -- must be greater than zero"),
		      option.nsteps->key, option.nsteps->answer);

    describe(option.map->answer, compact, no_data_str,
	     range, windowed, nsteps, as_int, flag.n->answer);

    return EXIT_SUCCESS;
}
