
/***************************************************************************
*
* MODULE:    r.buffer
*
* AUTHOR(S): Michael Shapiro, US Army Construction Engineering Research Laboratory
*	     James Westervelt, US Army CERL
*
* PURPOSE:   This program creates distance zones from non-zero
*	     cells in a grid layer. Distances are specified in
*	     meters (on the command-line). Window does not have to
*	     have square cells. Works both for planimetric (UTM,
*	     State Plane) and lat-long.
*
* COPYRIGHT: (c) 2006 by the GRASS Development Team
*
*	     This program is free software under the GNU General Public
*	     License (>=v2). Read the file COPYING that comes with GRASS
*	     for details.
*
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "distance.h"
#include "local_proto.h"
#include <grass/raster.h>
#include <grass/glocale.h>

struct Distance *distances;
int ndist;
int wrap_ncols;
MAPTYPE *map;
struct Cell_head window;
int minrow, maxrow, mincol, maxcol;
char *pgm_name;
double meters_to_grid = 1.0;
double ns_to_ew_squared;
int count_rows_with_data;

int main(int argc, char *argv[])
{
    struct Distance *pd;
    const char *input, *output, *mapset;
    char **zone_list;
    double to_meters;
    const char *units;
    int offset;
    int count;
    int step, nsteps;
    struct History hist;

    struct GModule *module;
    struct Option *opt1, *opt2, *opt3, *opt4;
    struct Flag *flag2;
    int ZEROFLAG;

    /* initialize GRASS */
    G_gisinit(argv[0]);

    pgm_name = argv[0];

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("buffer"));
    module->description =
	_("Creates a raster map showing buffer zones "
	  "surrounding cells that contain non-NULL category values.");

    opt1 = G_define_standard_option(G_OPT_R_INPUT);

    opt2 = G_define_standard_option(G_OPT_R_OUTPUT);

    opt3 = G_define_option();
    opt3->key = "distances";
    opt3->type = TYPE_DOUBLE;
    opt3->required = YES;
    opt3->multiple = YES;
    opt3->description = _("Distance zone(s)");

    opt4 = G_define_option();
    opt4->key = "units";
    opt4->options = "meters,kilometers,feet,miles,nautmiles";
    opt4->type = TYPE_STRING;
    opt4->required = NO;
    opt4->description = _("Units of distance");
    opt4->answer = "meters";

    flag2 = G_define_flag();
    flag2->key = 'z';
    flag2->description =
	_("Ignore zero (0) data cells instead of NULL cells");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    init_grass();

    /* get input, output map names */
    input = opt1->answer;
    output = opt2->answer;
    zone_list = opt3->answers;
    units = opt4->answer;

    ZEROFLAG = 0;		/* default: use NULL for non-data cells */
    ZEROFLAG = (flag2->answer);

    mapset = G_find_raster2(input, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), input);

    /* parse units */
    if (opt4->answer == NULL)
	units = "meters";

    if (strcmp(units, "meters") == 0)
	to_meters = 1.0;
    else if (strcmp(units, "feet") == 0)
	to_meters = FEET_TO_METERS;
    else if (strcmp(units, "kilometers") == 0)
	to_meters = KILOMETERS_TO_METERS;
    else if (strcmp(units, "miles") == 0)
	to_meters = MILES_TO_METERS;
    else if (strcmp(units, "nautmiles") == 0)
	to_meters = NAUT_MILES_TO_METERS;

    /* parse distances */
    if (!(count = parse_distances(zone_list, to_meters)))
	G_fatal_error(_("Parse distances error"));


    /* need to keep track of distance zones - in memory.
     * process MAX_DIST at a time
     *
     * Coding: 0 == not-yet determined, 1 == input cells,
     *         2 == distance zone #1,   3 == distance zone #2, etc.
     */

    read_input_map(input, mapset, ZEROFLAG);

    offset = 0;

    nsteps = (count - 1) / MAX_DIST + 1;

    pd = distances;
    for (step = 1; count > 0; step++) {
	if (nsteps > 1)
	    G_message(_("Pass %d (of %d)"), step, nsteps);
	ndist = count;
	if (ndist > MAX_DIST)
	    ndist = MAX_DIST;
	if (count_rows_with_data > 0)
	    execute_distance();
	write_output_map(output, offset);
	offset += ndist;
	distances += ndist;
	count -= ndist;
    }
    distances = pd;
    make_support_files(output, units);

    /* write map history (meta data) */
    Rast_short_history(output, "raster", &hist);
    Rast_set_history(&hist, HIST_DATSRC_1, input);
    Rast_append_format_history(&hist, "Buffer distance%s:", ndist > 1 ? "s" : "");
    Rast_append_format_history(&hist, " %s %s", opt3->answer, units);
    Rast_command_history(&hist);
    Rast_write_history(output, &hist);


    exit(EXIT_SUCCESS);
}
