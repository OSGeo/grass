
/****************************************************************************
 *
 * MODULE:       r.circle
 *
 * AUTHOR(S):    Bill Brown - CERL (Jan, 1993)
 *               Markus Neteler
 *
 * PURPOSE:      Creates a raster map containing concentric rings
 *	         around a given point.
 *
 * COPYRIGHT:    (C) 2006-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static double distance(double *, double *, double, double, int);

#ifndef HUGE_VAL
#define HUGE_VAL        1.7976931348623157e+308
#endif


int main(int argc, char *argv[])
{

    struct GModule *module;
    struct Option *coord, *out_file, *min, *max, *mult;
    struct Flag *flag;
    int *int_buf;
    struct Cell_head w;
    struct History history;
    int cellfile;
    double east, north, pt[2], cur[2], row, col, fmult;
    double fmin, fmax;
    int binary;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("buffer"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("circle"));
    module->description =
	_("Creates a raster map containing concentric "
	  "rings around a given point.");

    out_file = G_define_standard_option(G_OPT_R_OUTPUT);

    coord = G_define_standard_option(G_OPT_M_COORDS);
    coord->required = YES;
    coord->description = _("The coordinate of the center (east,north)");

    min = G_define_option();
    min->key = "min";
    min->type = TYPE_DOUBLE;
    min->required = NO;
    min->description = _("Minimum radius for ring/circle map (in meters)");

    max = G_define_option();
    max->key = "max";
    max->type = TYPE_DOUBLE;
    max->required = NO;
    max->description = _("Maximum radius for ring/circle map (in meters)");

    mult = G_define_option();
    mult->key = "multiplier";
    mult->type = TYPE_DOUBLE;
    mult->required = NO;
    mult->description = _("Data value multiplier");

    flag = G_define_flag();
    flag->key = 'b';
    flag->description = _("Generate binary raster map");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_scan_easting(coord->answers[0], &east, G_projection());
    G_scan_northing(coord->answers[1], &north, G_projection());
    pt[0] = east;
    pt[1] = north;

    fmult = 1.0;

    if (min->answer)
	sscanf(min->answer, "%lf", &fmin);
    else
	fmin = 0;

    if (max->answer)
	sscanf(max->answer, "%lf", &fmax);
    else
	fmax = HUGE_VAL;

    if (fmin > fmax)
	G_fatal_error(_("Please specify a radius in which min < max"));

    if (mult->answer)
	if (1 != sscanf(mult->answer, "%lf", &fmult))
	    fmult = 1.0;

    /* nonsense test */
    if (flag->answer && (!min->answer && !max->answer))
	G_fatal_error(_("Please specify min and/or max radius when "
			"using the binary flag"));

    if (flag->answer)
	binary = 1;		/* generate binary pattern only, useful for MASK */
    else
	binary = 0;

    G_get_set_window(&w);

    cellfile = Rast_open_c_new(out_file->answer);

    int_buf = (int *)G_malloc(w.cols * sizeof(int));
    {
	int c;

	for (row = 0; row < w.rows; row++) {
	    G_percent(row, w.rows, 2);
	    cur[1] = Rast_row_to_northing(row + 0.5, &w);
	    for (col = 0; col < w.cols; col++) {
		c = col;
		cur[0] = Rast_col_to_easting(col + 0.5, &w);
		int_buf[c] =
		    (int)(distance(pt, cur, fmin, fmax, binary) * fmult);
		if (int_buf[c] == 0)
		    Rast_set_null_value(&int_buf[c], 1, CELL_TYPE);
	    }
	    Rast_put_row(cellfile, int_buf, CELL_TYPE);

	}
    }
    G_free(int_buf);
    Rast_close(cellfile);
    Rast_short_history(out_file->answer, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(out_file->answer, &history);

    G_done_msg(_("Raster map <%s> created."),
	       out_file->answer);
    
    return (EXIT_SUCCESS);
}



/*******************************************************************/

static double distance(double from[2], double to[2], double min, double max,
		       int binary)
{
    static int first = 1;
    double dist;

    if (first) {
	first = 0;
	G_begin_distance_calculations();
    }

    dist = G_distance(from[0], from[1], to[0], to[1]);

    if ((dist >= min) && (dist <= max))
	if (!binary)
	    return dist;
	else
	    return 1;
    else
	return 0;		/* should be NULL ? */
}

/**********************************************************************/
