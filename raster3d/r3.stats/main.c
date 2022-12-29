/*
 *   r3.stats
 *
 *   Copyright (C) 2004-2017 by the GRASS Development Team
 *   Author(s): Soeren Gebbert
 *
 *   Purpose: Generates volume statistics for raster3d maps.           
 *
 *      This program is free software under the GNU General Public
 *      License (>=v2). Read the file COPYING that comes with GRASS
 *      for details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>
#include "local_proto.h"


int main(int argc, char *argv[])
{

    float val_f;		/* for misc use */
    double val_d;		/* for misc use */
    stat_table *stats = NULL;
    double min, max;
    equal_val_array *eqvals = NULL;

    unsigned int n = 0, nsteps;
    int map_type;
    char *infile = NULL;
    void *map = NULL;
    RASTER3D_Region region;
    unsigned int rows, cols, depths;
    unsigned int x, y, z;

    struct Option *inputfile, *steps;
    struct Flag *equal, *counts_only;
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("voxel"));
    G_add_keyword(_("volume"));
    module->description = _("Generates volume statistics for 3D raster maps.");

    /* Define the different options */

    inputfile = G_define_standard_option(G_OPT_R3_INPUT);

    steps = G_define_option();
    steps->key = "nsteps";
    steps->type = TYPE_INTEGER;
    steps->required = NO;
    steps->answer = "20";
    steps->description = _("Number of subranges to collect stats from");

    equal = G_define_flag();
    equal->key = 'e';
    equal->description =
	_("Calculate statistics based on equal value groups");

    counts_only = G_define_flag();
    counts_only->key = 'c';
    counts_only->description = _("Only print cell counts");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /*Set the defaults */
    Rast3d_init_defaults();

    /*get the current region */
    Rast3d_get_window(&region);

    cols = region.cols;
    rows = region.rows;
    depths = region.depths;

    sscanf(steps->answer, "%i", &nsteps);

    /* break if the wrong number of subranges are given */
    if (nsteps <= 0)
	G_fatal_error(_("The number of subranges has to be equal or greater than 1"));

    infile = inputfile->answer;

    if (NULL == G_find_raster3d(infile, ""))
	Rast3d_fatal_error(_("3D raster map <%s> not found"), infile);

    map =
	Rast3d_open_cell_old(infile, G_find_raster3d(infile, ""), &region,
			RASTER3D_TILE_SAME_AS_FILE, RASTER3D_USE_CACHE_DEFAULT);

    if (map == NULL)
	Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"), infile);

    map_type = Rast3d_tile_type_map(map);

    /* calculate statistics for groups of equal values */
    if ((equal->answer)) {

	/*search for equal values */
	eqvals = NULL;
	n = 0;
	for (z = 0; z < depths; z++) {
	    G_percent(z, depths - 1, 2);
	    for (y = 0; y < rows; y++) {
		for (x = 0; x < cols; x++) {
		    if (map_type == FCELL_TYPE) {
			Rast3d_get_value(map, x, y, z, &val_f, map_type);
			if (!Rast3d_is_null_value_num(&val_f, map_type)) {
			    /*the first entry */
			    if (eqvals == NULL)
				eqvals =
				    add_equal_val_to_array(eqvals,
							   (double)val_f);
			    else
				check_equal_value(eqvals, (double)val_f);

			    n++;	/*count non null cells */
			}
		    }
		    else if (map_type == DCELL_TYPE) {
			Rast3d_get_value(map, x, y, z, &val_d, map_type);
			if (!Rast3d_is_null_value_num(&val_d, map_type)) {
			    /*the first entry */
			    if (eqvals == NULL)
				eqvals =
				    add_equal_val_to_array(eqvals, val_d);
			    else
				check_equal_value(eqvals, val_d);

			    n++;	/*count non null cells */
			}
		    }
		}
	    }
	}

	if (eqvals) {
            /* sort the equal values array */
            G_message(_("Sort non-null values"));
            heapsort_eqvals(eqvals, eqvals->count);

            /* create the statistic table with equal values */
            stats = create_stat_table(eqvals->count, eqvals, 0, 0);
            /* compute the number of null values */
            stats->null->count = rows * cols * depths - n;

            free_equal_val_array(eqvals);
        }
    }
    else {

	/* create the statistic table based on value ranges */

	/* get the range of the map */
	Rast3d_range_load(map);
	Rast3d_range_min_max(map, &min, &max);

	stats = create_stat_table(nsteps, NULL, min, max);

	n = 0;
	for (z = 0; z < depths; z++) {
	    G_percent(z, depths - 1, 2);
	    for (y = 0; y < rows; y++) {
		for (x = 0; x < cols; x++) {
		    if (map_type == FCELL_TYPE) {
			Rast3d_get_value(map, x, y, z, &val_f, map_type);
			if (!Rast3d_is_null_value_num(&val_f, map_type)) {
			    check_range_value(stats, (double)val_f);
			    n++;
			}
		    }
		    else if (map_type == DCELL_TYPE) {
			Rast3d_get_value(map, x, y, z, &val_d, map_type);
			if (!Rast3d_is_null_value_num(&val_d, map_type)) {
			    check_range_value(stats, val_d);
			    n++;
			}
		    }
		}
	    }
	}
	/* compute the number of null values */
	stats->null->count = rows * cols * depths - n;
    }

    if(stats) {
        /* Compute the volume and percentage */
        update_stat_table(stats, &region);

        /* Print the statistics to stdout */
        print_stat_table(stats, counts_only->answer);

        free_stat_table(stats);
    }
    
    exit(EXIT_SUCCESS);
}
