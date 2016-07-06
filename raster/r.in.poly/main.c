
/****************************************************************************
 *
 * MODULE:       r.in.poly
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 * PURPOSE:      creates GRASS binary raster maps from ASCII files
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"


int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *input, *output, *title, *rows, *nulls, *type;
    int n;
    int raster_type;
    int null_value;
    int *null;
    null = &null_value;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    module->description =
	_("Creates raster maps from ASCII polygon/line/point data files.");


    input = G_define_standard_option(G_OPT_F_INPUT);
    input->description = _("Name of input file; or \"-\" to read from stdin");
 
    output = G_define_standard_option(G_OPT_R_OUTPUT);

    title = G_define_option();
    title->key = "title";
    title->key_desc = "phrase";
    title->type = TYPE_STRING;
    title->required = NO;
    title->description = _("Title for resultant raster map");

    type = G_define_standard_option(G_OPT_R_TYPE);
    type->required = NO;
    type->answer = "CELL";

    nulls = G_define_option();
    nulls->key = "null";
    nulls->type = TYPE_INTEGER;
    nulls->required = NO;
    nulls->description = _("Integer representing NULL value data cell");

    rows = G_define_option();
    rows->key = "rows";
    rows->type = TYPE_INTEGER;
    rows->required = NO;
    rows->description = _("Number of rows to hold in memory");
    rows->answer = "4096";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    sscanf(rows->answer, "%d", &n);
    if (n < 1)
	G_fatal_error(_("Minimum number of rows to hold in memory is 1"));

    if (strcmp(type->answer, "CELL") == 0)
        raster_type = CELL_TYPE;
    else if (strcmp(type->answer, "FCELL") == 0)
        raster_type = FCELL_TYPE;
    else if (strcmp(type->answer, "DCELL") == 0)
        raster_type = DCELL_TYPE;
    else
        G_fatal_error(_("Type doesn't exist"));

    if (nulls->answer)
        *null = atoi(nulls->answer);
    else
        null = NULL;

    exit(poly_to_rast(input->answer, output->answer, title->answer, n, raster_type, null));
}
