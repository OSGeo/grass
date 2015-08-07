
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
#include <grass/raster.h>
#include <grass/glocale.h>

#include "method.h"

/* modify this table to add new methods */
struct menu menu[] = {
    {"diversity", o_divr,    "Diversity of values in specified objects in %%"},
    {"average",   o_average, "Average of values in specified objects"},
    {"mode",      o_mode,    "Mode of values in specified objects"},
    {"median",    o_median,  "Median of values in specified objects"},
    {"avedev",    o_adev,    "Average deviation of values in specified objects"},
    {"stddev",    o_sdev,    "Standard deviation of values in specified objects"},
    {"variance",  o_var,     "Variance of values in specified objects"},
    {"skewness",  o_skew,    "Skewnes of values in specified objects"},
    {"kurtosis",  o_kurt,    "Kurtosis of values in specified objects"},
    {"min",       o_min,     "Minimum of values in specified objects"},
    {"max",       o_max,     "Maximum of values in specified objects"},
    {"sum",       o_sum,     "Sum of values in specified objects"},
    {NULL,        NULL,      NULL}
};

int main(int argc, char **argv)
{
    int o_method;
    struct GModule *module;
    struct Option *method, *basemap, *covermap, *outputmap;
    struct Flag *flag_c;
    struct Categories cats;
    char methods[1024];

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("zonal statistics"));
    module->description =
	_("Calculates category or object oriented statistics.");

    basemap = G_define_standard_option(G_OPT_R_BASE);

    covermap = G_define_standard_option(G_OPT_R_COVER);

    method = G_define_option();
    method->key = "method";
    method->type = TYPE_STRING;
    method->required = YES;
    method->description = _("Method of object-based statistic");

    for (o_method = 0; menu[o_method].name; o_method++) {
	if (o_method)
	    strcat(methods, ",");
	else
	    *(methods) = 0;
	strcat(methods, menu[o_method].name);
    }
    method->options = G_store(methods);

    for (o_method = 0; menu[o_method].name; o_method++) {
	if (o_method)
	    strcat(methods, ";");
	else
	    *(methods) = 0;
	strcat(methods, menu[o_method].name);
	strcat(methods, ";");
	strcat(methods, menu[o_method].text);
    }
    method->descriptions = G_store(methods);

    outputmap = G_define_standard_option(G_OPT_R_OUTPUT);
    outputmap->description = _("Resultant raster map");
    outputmap->required = YES;

    flag_c = G_define_flag();
    flag_c->key = 'c';
    flag_c->description =
	_("Cover values extracted from the category labels of the cover map");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (Rast_map_is_fp(basemap->answer, "") != 0)
	G_fatal_error(_("This module currently only works for integer (CELL) maps"));

    if (Rast_map_is_fp(covermap->answer, "") != 0)
	G_fatal_error(_("This module currently only works for integer (CELL) maps"));

    if (Rast_read_cats(covermap->answer, "", &cats) < 0)
	G_fatal_error(_("Unable to read category file of raster map <%s>"),
		      covermap->answer);

    for (o_method = 0; menu[o_method].name; o_method++)
	if (strcmp(menu[o_method].name, method->answer) == 0)
	    break;

    if (!menu[o_method].name) {
	G_warning(_("<%s=%s> unknown %s"), method->key, method->answer,
		  method->key);
	G_usage();

	exit(EXIT_FAILURE);
    }

    (*menu[o_method].func)(basemap->answer, covermap->answer,
			   outputmap->answer,
			   flag_c->answer, &cats);

    return 0;
}

