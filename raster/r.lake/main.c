
/****************************************************************************
 *
 * MODULE:       r.lake
 *
 * AUTHOR(S):    Maris Nartiss - maris.nartiss gmail.com
 *               with hints from friendly GRASS dev team
 *
 * PURPOSE:      Fills lake with water at given height above DEM.
 *               As seed You can use already existing map or
 *               X,Y coordinates.
 *
 * COPYRIGHT:    (C) 2005-2008, 2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *  TODO:        - Option to create 3D output;
 *               - Test with lat/lon location, feets and other crap;
 *               - In progress: Add different debug level messages;
 *               - In progress: Option to output resulting lake area and volume.
 *
 *  BUGS:        - Lake (seed) map cannot be negative!
 *               - Negative output (-n) maps cannot be used as input.
 *               - Code is not large file safe (i.e. array with whole map).
 *
 *****************************************************************************/

/* You are not allowed to remove this comment block. /M. Nartiss/
 *
 *  Kaarliit, shii programma ir veltiita Tev.
 *
 */
#include <stdio.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/* Saves map file from 2d array. NULL must be 0. Also meanwhile calculates area and volume. */
void save_map(FCELL ** out, int out_fd, int rows, int cols, int flag,
	      FCELL * min_depth, FCELL * max_depth, double *area,
	      double *volume)
{
    int row, col;
    double cellsize = -1;

    G_debug(1, "Saving new map");

    if (G_begin_cell_area_calculations() == 0 || G_begin_cell_area_calculations() == 1) {	/* All cells have constant size... */
	cellsize = G_area_of_cell_at_row(0);
    }
    G_debug(1, "Cell area: %f", cellsize);

    for (row = 0; row < rows; row++) {
	if (cellsize == -1)	/* Get LatLon current rows cell size */
	    cellsize = G_area_of_cell_at_row(row);
	for (col = 0; col < cols; col++) {
	    if (flag == 1)	/* Create negative map */
		out[row][col] = 0 - out[row][col];
	    if (out[row][col] == 0) {
		Rast_set_f_null_value(&out[row][col], 1);
	    }
	    if (out[row][col] > 0 || out[row][col] < 0) {
		G_debug(5, "volume %f += cellsize %f  * value %f [%d,%d]",
			*volume, cellsize, out[row][col], row, col);
		*area += cellsize;
		*volume += cellsize * out[row][col];
	    }

	    /* Get min/max depth. Can be useful ;) */
	    if (out[row][col] > *max_depth)
		*max_depth = out[row][col];
	    if (out[row][col] < *min_depth)
		*min_depth = out[row][col];
	}
	Rast_put_f_row(out_fd, out[row]);
	G_percent(row + 1, rows, 5);
    }
}

/* Check water presence in sliding window */
short is_near_water(FCELL window[][3])
{
    int i, j;

    /* If center is under water */
    if (window[1][1] > 0)
	return 1;

    for (i = 0; i < 3; i++) {
	for (j = 0; j < 3; j++) {
	    if (window[i][j] > 0)
		return 1;
	}
    }
    return 0;
}

/* Loads values into window around central cell */
void load_window_values(FCELL ** in_rows, FCELL window[][3],
			int rows, int cols, int row, int col)
{
    int i, j;

    rows -= 1;			/* Row'n'Col count starts from 0! */
    cols -= 1;

    for (i = -1; i < 2; i++) {
	if (row + i < 0 || row + i > rows) {	/* First or last line... */
	    window[i + 1][0] = 0;
	    window[i + 1][1] = 0;
	    window[i + 1][2] = 0;
	    continue;
	}
	else {
	    for (j = -1; j < 2; j++) {
		if (col + j < 0 || col + j > cols - 1) {	/* First or last column... */
		    window[i + 1][j + 1] = 0;
		    continue;
		}
		else {		/* All normal cases... */
		    window[i + 1][j + 1] = in_rows[row + i][col + j];
		}
	    }
	}
    }
}

int main(int argc, char *argv[])
{
    char *terrainmap, *seedmap, *lakemap;
    int rows, cols, in_terran_fd, out_fd, lake_fd, row, col, pases, pass;
    int lastcount, curcount, start_col = 0, start_row = 0;
    double east, north, area = 0, volume = 0;
    FCELL **in_terran, **out_water, water_level, max_depth = 0, min_depth = 0;
    FCELL water_window[3][3];
    struct Option *tmap_opt, *smap_opt, *wlvl_opt, *lake_opt, *sdxy_opt;
    struct Flag *negative_flag, *overwrite_flag;
    struct GModule *module;
    struct Colors colr;
    struct Cell_head window;
    struct History history;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("hazard"));
    G_add_keyword(_("flood"));
    module->description = _("Fills lake at given point to given level.");

    tmap_opt = G_define_standard_option(G_OPT_R_ELEV);

    wlvl_opt = G_define_option();
    wlvl_opt->key = "water_level";
    wlvl_opt->description = _("Water level");
    wlvl_opt->type = TYPE_DOUBLE;
    wlvl_opt->required = YES;

    lake_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    lake_opt->key = "lake";
    lake_opt->required = NO;
    lake_opt->guisection = _("Output");

    sdxy_opt = G_define_standard_option(G_OPT_M_COORDS);
    sdxy_opt->label = _("Seed point coordinates");
    sdxy_opt->description = _("Either this coordinates pair or a seed"
	" map have to be specified");
    sdxy_opt->required = NO;
    sdxy_opt->multiple = NO;
    sdxy_opt->guisection = _("Seed");

    smap_opt = G_define_standard_option(G_OPT_R_MAP);
    smap_opt->key = "seed";
    smap_opt->label =
	_("Input raster map with given starting point(s) (at least 1 cell > 0)");
    smap_opt->description =
	_("Either this parameter or a coordinates pair have to be specified");
    smap_opt->required = NO;
    smap_opt->guisection = _("Seed");

    negative_flag = G_define_flag();
    negative_flag->key = 'n';
    negative_flag->description =
	_("Use negative depth values for lake raster map");

    overwrite_flag = G_define_flag();
    overwrite_flag->key = 'o';
    overwrite_flag->description =
	_("Overwrite seed map with result (lake) map");
    overwrite_flag->guisection = _("Output");

    if (G_parser(argc, argv))	/* Returns 0 if successful, non-zero otherwise */
	exit(EXIT_FAILURE);

    if (smap_opt->answer && sdxy_opt->answer)
	G_fatal_error(_("Both seed map and coordinates cannot be specified"));

    if (!smap_opt->answer && !sdxy_opt->answer)
	G_fatal_error(_("Seed map or seed coordinates must be set!"));

    if (sdxy_opt->answer && !lake_opt->answer)
	G_fatal_error(_("Seed coordinates and output map lake= must be set!"));

    if (lake_opt->answer && overwrite_flag->answer)
	G_fatal_error(_("Both lake and overwrite cannot be specified"));

    if (!lake_opt->answer && !overwrite_flag->answer)
	G_fatal_error(_("Output lake map or overwrite flag must be set!"));

    terrainmap = tmap_opt->answer;
    seedmap = smap_opt->answer;
    sscanf(wlvl_opt->answer, "%f", &water_level);
    lakemap = lake_opt->answer;

    /* If lakemap is set, write to it, else is set overwrite flag and we should write to seedmap. */
    if (lakemap)
	lake_fd = Rast_open_new(lakemap, 1);

    rows = Rast_window_rows();
    cols = Rast_window_cols();

    /* If we use x,y as seed... */
    if (sdxy_opt->answer) {
	G_get_window(&window);
	east = window.east;
	north = window.north;

	G_scan_easting(sdxy_opt->answers[0], &east, G_projection());
	G_scan_northing(sdxy_opt->answers[1], &north, G_projection());
	start_col = (int)Rast_easting_to_col(east, &window);
	start_row = (int)Rast_northing_to_row(north, &window);

	if (start_row < 0 || start_row > rows ||
	    start_col < 0 || start_col > cols)
	    G_fatal_error(_("Seed point outside the current region"));
    }

    /* Open terrain map */
    in_terran_fd = Rast_open_old(terrainmap, "");

    /* Open seed map */
    if (smap_opt->answer)
	out_fd = Rast_open_old(seedmap, "");

    /* Pointers to rows. Row = ptr to 'col' size array. */
    in_terran = (FCELL **) G_malloc(rows * sizeof(FCELL *));
    out_water = (FCELL **) G_malloc(rows * sizeof(FCELL *));
    if (in_terran == NULL || out_water == NULL)
	G_fatal_error(_("G_malloc: out of memory"));


    G_debug(1, "Loading maps...");
    /* foo_rows[row] == array with data (2d array). */
    for (row = 0; row < rows; row++) {
	in_terran[row] = (FCELL *) G_malloc(cols * sizeof(FCELL));
	out_water[row] = (FCELL *) G_calloc(cols, sizeof(FCELL));

	/* In newly created space load data from file. */
	Rast_get_f_row(in_terran_fd, in_terran[row], row);

	if (smap_opt->answer)
	    Rast_get_f_row(out_fd, out_water[row], row);

	G_percent(row + 1, rows, 5);
    }

    /* Set seed point */
    if (sdxy_opt->answer)
	/* Check is water level higher than seed point */
	if (in_terran[start_row][start_col] >= water_level)
	    G_fatal_error(_("Given water level at seed point is below earth surface. "
			   "Increase water level or move seed point."));
    out_water[start_row][start_col] = 1;

    /* Close seed map for reading. */
    if (smap_opt->answer)
	Rast_close(out_fd);

    /* Open output map for writing. */
    if (lakemap)
	out_fd = lake_fd;
    else
	out_fd = Rast_open_new(seedmap, 1);

    /* More pases are renudant. Real pases count is controlled by altered cell count. */
    pases = (int)(rows * cols) / 2;

    G_debug(1,
	    "Starting lake filling at level of %8.4f in %d passes. Percent done:",
	    water_level, pases);

    lastcount = 0;

    for (pass = 0; pass < pases; pass++) {
	G_debug(3, "Pass: %d", pass);
	curcount = 0;
	/* Move from left upper corner to right lower corner. */
	for (row = 0; row < rows; row++) {
	    for (col = 0; col < cols; col++) {
		/* Loading water data into window. */
		load_window_values(out_water, water_window, rows, cols, row,
				   col);

		/* Cheking presence of water. */
		if (is_near_water(water_window) == 1) {
		    if (in_terran[row][col] < water_level) {
			out_water[row][col] =
			    water_level - in_terran[row][col];
			curcount++;
		    }
		    else {
			out_water[row][col] = 0;	/* Cell is higher than water level -> NULL. */
		    }
		}
	    }
	}
	if (curcount == lastcount)
	    break;		/* We done. */
	lastcount = curcount;
	curcount = 0;
	/* Move backwards - from lower right corner to upper left corner. */
	for (row = rows - 1; row >= 0; row--) {
	    for (col = cols - 1; col >= 0; col--) {
		load_window_values(out_water, water_window, rows, cols, row,
				   col);

		if (is_near_water(water_window) == 1) {
		    if (in_terran[row][col] < water_level) {
			out_water[row][col] =
			    water_level - in_terran[row][col];
			curcount++;
		    }
		    else {
			out_water[row][col] = 0;
		    }
		}
	    }
	}
	G_percent(pass + 1, pases, 10);
	if (curcount == lastcount)
	    break;		/* We done. */
	lastcount = curcount;
    }				/*pases */

    G_percent(pases, pases, 10);	/* Show 100%. */

    save_map(out_water, out_fd, rows, cols, negative_flag->answer, &min_depth,
	     &max_depth, &area, &volume);

    G_message(_("Lake depth from %f to %f (specified water level is taken as zero)"), min_depth, max_depth);
    G_message(_("Lake area %f square meters"), area);
    G_message(_("Lake volume %f cubic meters"), volume);
    G_important_message(_("Volume is correct only if lake depth (terrain raster map) is in meters"));

    /* Close all files. Lake map gets written only now. */
    Rast_close(in_terran_fd);
    Rast_close(out_fd);

    /* Add blue color gradient from light bank to dark depth */
    Rast_init_colors(&colr);
    if (negative_flag->answer == 1) {
	Rast_add_f_color_rule(&max_depth, 0, 240, 255,
				  &min_depth, 0, 50, 170, &colr);
    }
    else {
	Rast_add_f_color_rule(&min_depth, 0, 240, 255,
				  &max_depth, 0, 50, 170, &colr);
    }

    Rast_write_colors(lakemap, G_mapset(), &colr);

    Rast_short_history(lakemap, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(lakemap, &history);

    return EXIT_SUCCESS;
}
