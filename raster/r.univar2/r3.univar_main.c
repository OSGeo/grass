/*
 * r3.univar
 *
 *  Calculates univariate statistics from the non-null 3d cells of a raster3d map
 *
 *   Copyright (C) 2004-2007 by the GRASS Development Team
 *   Author(s): Soeren Gebbert
 *              Based on r.univar from Hamish Bowman, University of Otago, New Zealand
 *              and Martin Landa
 *              heapsort code from http://de.wikipedia.org/wiki/Heapsort
 *
 *      This program is free software under the GNU General Public
 *      License (>=v2). Read the file COPYING that comes with GRASS
 *      for details.
 *
 */

#define MAIN
#include "globals.h"

/* local proto */
void set_params();

/* ************************************************************************* */
/* Set up the arguments we are expecting ********************************** */
/* ************************************************************************* */
void set_params()
{
    param.inputfile = G_define_standard_option(G_OPT_R3_INPUT);

    param.percentile = G_define_option();
    param.percentile->key = "percentile";
    param.percentile->type = TYPE_INTEGER;
    param.percentile->required = NO;
    param.percentile->multiple = YES;
    param.percentile->options = "0-100";
    param.percentile->answer = "90";
    param.percentile->description =
	_("Percentile to calculate (requires extended statistics flag)");

    param.shell_style = G_define_flag();
    param.shell_style->key = 'g';
    param.shell_style->description = _("Print the stats in shell script style");

    param.extended = G_define_flag();
    param.extended->key = 'e';
    param.extended->description = _("Calculate extended statistics");

    return;
}


/* *************************************************************** */
/* **** the main functions for r3.univar ************************* */
/* *************************************************************** */
int main(int argc, char *argv[])
{

    float val_f;		/* for misc use */
    double val_d;		/* for misc use */
    int first = TRUE;		/* min/max init flag */

    int map_type;
    univar_stat *stats;

    char *infile;
    void *map;
    G3D_Region region;
    unsigned int i;
    unsigned int rows, cols, depths;
    unsigned int x, y, z;

    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster3d, statistics");
    module->description =
	_("Calculates univariate statistics from the non-null 3d cells of a raster3d map.");

    /* Define the different options */
    set_params();

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /*Set the defaults */
    G3d_initDefaults();

    /*get the current region */
    G3d_getWindow(&region);

    cols = region.cols;
    rows = region.rows;
    depths = region.depths;


    infile = param.inputfile->answer;

    if (NULL == G_find_grid3(infile, ""))
	G3d_fatalError(_("Requested g3d map <%s> not found"), infile);

    /*Open all maps with default region */
    map =
	G3d_openCellOld(infile, G_find_grid3(infile, ""), &region,
			G3D_TILE_SAME_AS_FILE, G3D_USE_CACHE_DEFAULT);

    if (map == NULL)
	G3d_fatalError(_("Error opening g3d map <%s>"), infile);

    map_type = G3d_tileTypeMap(map);

    i = 0;
    while(param.percentile->answers[i])
	i++;
    stats = create_univar_stat_struct(map_type, cols * rows * depths, i);
    for(i = 0; i < stats->n_perc; i++) {
	sscanf(param.percentile->answers[i], "%i", &stats->perc[i]);
    }

    stats->n = 0;
    for (z = 0; z < depths; z++) {	/*From the bottom to the top */
	if (!(param.shell_style->answer))
	    G_percent(z, depths - 1, 10);
	for (y = 0; y < rows; y++) {
	    for (x = 0; x < cols; x++) {
		if (map_type == FCELL_TYPE) {
		    G3d_getValue(map, x, y, z, &val_f, map_type);
		    if (!G3d_isNullValueNum(&val_f, map_type)) {
			if (param.extended->answer)
			    stats->fcell_array[stats->n] = val_f;

			stats->sum += val_f;
			stats->sumsq += (val_f * val_f);
			stats->sum_abs += fabs(val_f);

			if (first) {
			    stats->max = val_f;
			    stats->min = val_f;
			    first = FALSE;
			}
			else {
			    if (val_f > stats->max)
				stats->max = val_f;
			    if (val_f < stats->min)
				stats->min = val_f;
			}
			stats->n++;
		    }
		}
		else if (map_type == DCELL_TYPE) {
		    G3d_getValue(map, x, y, z, &val_d, map_type);
		    if (!G3d_isNullValueNum(&val_d, map_type)) {
			if (param.extended->answer)
			    stats->dcell_array[stats->n] = val_d;

			stats->sum += val_d;
			stats->sumsq += val_d * val_d;
			stats->sum_abs += fabs(val_d);

			if (first) {
			    stats->max = val_d;
			    stats->min = val_d;
			    first = FALSE;
			}
			else {
			    if (val_d > stats->max)
				stats->max = val_d;
			    if (val_d < stats->min)
				stats->min = val_d;
			}
			stats->n++;
		    }
		}
	    }
	}
    }

    /* create the output */
    print_stats(stats);

    /* release memory */
    free_univar_stat_struct(stats);

    exit(EXIT_SUCCESS);
}
