
/****************************************************************************
 *
 * MODULE:       r.statistics
 *               
 * AUTHOR(S):    Martin Schroeder, Geographisches Institut Heidelberg, Germany
 *
 * PURPOSE:      Category or object oriented statistics
 *
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#define MAIN
#include "method.h"

/* function prototypes */
static int is_ok(char *, char *);

int main(int argc, char **argv)
{
    char *mapset;
    int o_method;
    struct GModule *module;
    struct Option *method, *basemap, *covermap, *outputmap;
    struct Flag *flag_c;
    struct Categories cats;
    char methods[1024];

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster, statistics");
    module->description =
	_("Calculates category or object oriented statistics.");

    basemap = G_define_standard_option(G_OPT_R_BASE);

    covermap = G_define_standard_option(G_OPT_R_COVER);

    for (o_method = 0; menu[o_method].name; o_method++) {
	if (o_method)
	    strcat(methods, ",");
	else
	    *(methods) = 0;
	strcat(methods, menu[o_method].name);
    }

    method = G_define_option();
    method->key = "method";
    method->type = TYPE_STRING;
    method->required = YES;
    method->description = _("Method of object-based statistic");
    method->options = methods;

    outputmap = G_define_standard_option(G_OPT_R_OUTPUT);
    outputmap->description =
	_("Resultant raster map (not used with 'distribution')");
    outputmap->required = NO;

    flag_c = G_define_flag();
    flag_c->key = 'c';
    flag_c->description =
	_("Cover values extracted from the category labels of the cover map");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if ((mapset = G_find_cell2(basemap->answer, "")) == 0)
	G_fatal_error(_("Raster map <%s> not found"), basemap->answer);

    if (G_raster_map_is_fp(basemap->answer, mapset) != 0)
	G_fatal_error(_("This module currently only works for integer (CELL) maps"));

    if ((mapset = G_find_cell2(covermap->answer, "")) == 0)
	G_fatal_error(_("Raster map <%s> not found"), covermap->answer);

    if (G_raster_map_is_fp(covermap->answer, mapset) != 0)
	G_fatal_error(_("This module currently only works for integer (CELL) maps"));

    if (G_read_cats(covermap->answer, mapset, &cats) < 0) {
	G_fatal_error(_("Unable to read category file of raster map <%s@%s>"),
		      covermap->answer, mapset);
    }

    for (o_method = 0; menu[o_method].name; o_method++)
	if (strcmp(menu[o_method].name, method->answer) == 0)
	    break;

    if (!menu[o_method].name) {
	G_warning(_("<%s=%s> unknown %s"), method->key, method->answer,
		  method->key);
	G_usage();

	exit(EXIT_FAILURE);
    }

    switch (menu[o_method].val) {
    case DISTRIB:
	if (outputmap->answer != NULL)
	    G_warning(_("Output map <%s> ignored"), outputmap->answer);

	o_distrib(basemap->answer, covermap->answer,
		  outputmap->answer, flag_c->answer);
	break;
    case AVERAGE:
	is_ok(method->answer, outputmap->answer);
	o_average(basemap->answer, covermap->answer,
		  outputmap->answer, flag_c->answer, &cats);
	break;
    case MODE:
	is_ok(method->answer, outputmap->answer);
	o_mode(basemap->answer, covermap->answer,
	       outputmap->answer, flag_c->answer, &cats);
	break;
    case ADEV:
	is_ok(method->answer, outputmap->answer);
	o_adev(basemap->answer, covermap->answer,
	       outputmap->answer, flag_c->answer, &cats);
	break;
    case SDEV:
	is_ok(method->answer, outputmap->answer);
	o_sdev(basemap->answer, covermap->answer,
	       outputmap->answer, flag_c->answer, &cats);
	break;
    case VARIANC:
	is_ok(method->answer, outputmap->answer);
	o_var(basemap->answer, covermap->answer,
	      outputmap->answer, flag_c->answer, &cats);
	break;
    case SKEWNES:
	is_ok(method->answer, outputmap->answer);
	o_skew(basemap->answer, covermap->answer,
	       outputmap->answer, flag_c->answer, &cats);
	break;
    case KURTOSI:
	is_ok(method->answer, outputmap->answer);
	o_kurt(basemap->answer, covermap->answer,
	       outputmap->answer, flag_c->answer, &cats);
	break;
    case MEDIAN:
	is_ok(method->answer, outputmap->answer);
	o_median(basemap->answer, covermap->answer,
		 outputmap->answer, flag_c->answer, &cats);
	break;
    case MIN:
	is_ok(method->answer, outputmap->answer);
	o_min(basemap->answer, covermap->answer,
	      outputmap->answer, flag_c->answer, &cats);
	break;
    case MAX:
	is_ok(method->answer, outputmap->answer);
	o_max(basemap->answer, covermap->answer,
	      outputmap->answer, flag_c->answer, &cats);
	break;
    case SUM:
	is_ok(method->answer, outputmap->answer);
	o_sum(basemap->answer, covermap->answer,
	      outputmap->answer, flag_c->answer, &cats);
	break;
    case DIV:
	is_ok(method->answer, outputmap->answer);
	o_divr(basemap->answer, covermap->answer,
	       outputmap->answer, flag_c->answer, &cats);
	break;

    default:
	G_fatal_error(_("Not yet implemented!"));
    }

    return 0;
}


static int is_ok(char *method, char *map)
{
    if (map == NULL)
	G_fatal_error(_("An output raster map needs to be defined with method '%s'"),
		      method);

    return 0;
}
