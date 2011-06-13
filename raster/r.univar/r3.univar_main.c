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
 *              Zonal stats: Markus Metz
 *
 *      This program is free software under the GNU General Public
 *      License (>=v2). Read the file COPYING that comes with GRASS
 *      for details.
 *
 */

#include <string.h>
#include "globals.h"
#include "grass/G3d.h"

param_type param;
zone_type zone_info;

/* ************************************************************************* */
/* Set up the arguments we are expecting ********************************** */
/* ************************************************************************* */
void set_params()
{
    param.inputfile = G_define_standard_option(G_OPT_R3_INPUT);

    param.zonefile = G_define_standard_option(G_OPT_R3_INPUT);
    param.zonefile->key = "zones";
    param.zonefile->required = NO;
    param.zonefile->description =
	_("3D Raster map used for zoning, must be of type CELL");

    param.output_file = G_define_standard_option(G_OPT_F_OUTPUT);
    param.output_file->required = NO;
    param.output_file->description =
	_("Name for output file (if omitted or \"-\" output to stdout)");

    param.percentile = G_define_option();
    param.percentile->key = "percentile";
    param.percentile->type = TYPE_DOUBLE;
    param.percentile->required = NO;
    param.percentile->multiple = YES;
    param.percentile->options = "0-100";
    param.percentile->answer = "90";
    param.percentile->description =
	_("Percentile to calculate (requires extended statistics flag)");

    param.separator = G_define_standard_option(G_OPT_F_SEP);
    param.separator->description = _("Special characters: space, comma, tab");

    param.shell_style = G_define_flag();
    param.shell_style->key = 'g';
    param.shell_style->description =
	_("Print the stats in shell script style");

    param.extended = G_define_flag();
    param.extended->key = 'e';
    param.extended->description = _("Calculate extended statistics");

    param.table = G_define_flag();
    param.table->key = 't';
    param.table->description = _("Table output format instead of standard output format");

    return;
}


/* *************************************************************** */
/* **** the main functions for r3.univar ************************* */
/* *************************************************************** */
int main(int argc, char *argv[])
{

    CELL val_c;			/* for misc use */
    FCELL val_f;		/* for misc use */
    DCELL val_d;		/* for misc use */
    int map_type, zmap_type;
    univar_stat *stats;

    char *infile, *zonemap;
    void *map, *zmap = NULL;
    G3D_Region region;
    unsigned int i;
    unsigned int rows, cols, depths;
    unsigned int x, y, z;
    double dmin, dmax;
    int zone, n_zones, use_zone = 0;
    char *mapset, *name;
    struct FPRange zone_range;

    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("statistics"));
    module->description =
	_("Calculates univariate statistics from the non-null 3d cells of a raster3d map.");

    /* Define the different options */
    set_params();

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Set the defaults */
    G3d_initDefaults();

    /* get the current region */
    G3d_getWindow(&region);

    cols = region.cols;
    rows = region.rows;
    depths = region.depths;

    name = param.output_file->answer;
    if (name != NULL && strcmp(name, "-") != 0) {
	if (NULL == freopen(name, "w", stdout)) {
	    G_fatal_error(_("Unable to open file <%s> for writing"), name);
	}
    }

    /* table field separator */
    zone_info.sep = param.separator->answer;
    if (strcmp(zone_info.sep, "\\t") == 0)
	zone_info.sep = "\t";
    if (strcmp(zone_info.sep, "tab") == 0)
	zone_info.sep = "\t";
    if (strcmp(zone_info.sep, "space") == 0)
	zone_info.sep = " ";
    if (strcmp(zone_info.sep, "comma") == 0)
	zone_info.sep = ",";

    dmin = 0.0 / 0.0;	/* set to nan as default */
    dmax = 0.0 / 0.0;	/* set to nan as default */
    zone_info.min = 0.0 / 0.0;	/* set to nan as default */
    zone_info.max = 0.0 / 0.0;	/* set to nan as default */
    zone_info.n_zones = 0;

    /* open 3D zoning raster with default region */
    if ((zonemap = param.zonefile->answer) != NULL) {
	if (NULL == (mapset = G_find_grid3(zonemap, "")))
	    G3d_fatalError(_("Requested g3d map <%s> not found"), zonemap);

	zmap =
	    G3d_openCellOld(zonemap, G_find_grid3(zonemap, ""), &region,
			    G3D_TILE_SAME_AS_FILE, G3D_USE_CACHE_DEFAULT);

	if (zmap == NULL)
	    G3d_fatalError(_("Error opening g3d map <%s>"), zonemap);

	zmap_type = G3d_tileTypeMap(zmap);
	
	if (zmap_type != CELL_TYPE)
	    G_fatal_error("Zoning raster must be of type CELL");

	if (G3d_readRange(zonemap, mapset, &zone_range) == -1)
	    G_fatal_error("Can not read range for zoning raster");
	G3d_range_min_max(zmap, &dmin, &dmax);
	if (G3d_readCats(zonemap, mapset, &(zone_info.cats)))
	    G_warning("no category support for zoning raster");

	/* properly round dmin and dmax */
	if (dmin < 0)
	    zone_info.min = dmin - 0.5;
	else
	    zone_info.min = dmin + 0.5;
	if (dmax < 0)
	    zone_info.max = dmax - 0.5;
	else
	    zone_info.max = dmax + 0.5;
	    
	zone_info.n_zones = zone_info.max - zone_info.min + 1;

	use_zone = 1;
    }

    /* Open 3D input raster with default region */
    infile = param.inputfile->answer;

    if (NULL == G_find_grid3(infile, ""))
	G3d_fatalError(_("Requested g3d map <%s> not found"), infile);

    map =
	G3d_openCellOld(infile, G_find_grid3(infile, ""), &region,
			G3D_TILE_SAME_AS_FILE, G3D_USE_CACHE_DEFAULT);

    if (map == NULL)
	G3d_fatalError(_("Error opening g3d map <%s>"), infile);

    map_type = G3d_tileTypeMap(map);

    i = 0;
    while (param.percentile->answers[i])
	i++;
 
    n_zones = zone_info.n_zones;

    if (n_zones == 0)
        n_zones = 1;

    stats = create_univar_stat_struct(map_type, i);
    for (i = 0; i < n_zones; i++) {
	unsigned int j;
	for (j = 0; j < stats[i].n_perc; j++) {
	    sscanf(param.percentile->answers[j], "%lf", &(stats[i].perc[j]));
	}
    }

    for (z = 0; z < depths; z++) {	/* From the bottom to the top */
	if (!(param.shell_style->answer))
	    G_percent(z, depths - 1, 10);
	for (y = 0; y < rows; y++) {
	    for (x = 0; x < cols; x++) {
		zone = 0;
		if (zone_info.n_zones) {
		    G3d_getValue(zmap, x, y, z, &zone, CELL_TYPE);
		    if (G3d_isNullValueNum(&zone, CELL_TYPE))
			continue;
                    zone -= zone_info.min;
                }
		if (map_type == CELL_TYPE) {
		    G3d_getValue(map, x, y, z, &val_c, map_type);
		    if (!G3d_isNullValueNum(&val_c, map_type)) {
			if (param.extended->answer) {
			    if (stats[zone].n >= stats[zone].n_alloc) {
				size_t msize;
				stats[zone].n_alloc += 1000;
				msize = stats[zone].n_alloc * sizeof(CELL);
				stats[zone].cell_array =
				    (CELL *)G_realloc((void *)stats[zone].cell_array, msize);
			    }

			    stats[zone].cell_array[stats[zone].n] = val_c;
			}

			stats[zone].sum += val_c;
			stats[zone].sumsq += (val_c * val_c);
			stats[zone].sum_abs += fabs(val_c);

			if (stats[zone].first) {
			    stats[zone].max = val_c;
			    stats[zone].min = val_c;
			    stats[zone].first = FALSE;
			}
			else {
			    if (val_c > stats[zone].max)
				stats[zone].max = val_c;
			    if (val_c < stats[zone].min)
				stats[zone].min = val_c;
			}
			stats[zone].n++;
		    }
		    stats[zone].size++;
		}
		else if (map_type == FCELL_TYPE) {
		    G3d_getValue(map, x, y, z, &val_f, map_type);
		    if (!G3d_isNullValueNum(&val_f, map_type)) {
			if (param.extended->answer) {
			    if (stats[zone].n >= stats[zone].n_alloc) {
				size_t msize;
				stats[zone].n_alloc += 1000;
				msize = stats[zone].n_alloc * sizeof(FCELL);
				stats[zone].fcell_array =
				    (FCELL *)G_realloc((void *)stats[zone].fcell_array, msize);
			    }

			    stats[zone].fcell_array[stats[zone].n] = val_f;
			}

			stats[zone].sum += val_f;
			stats[zone].sumsq += (val_f * val_f);
			stats[zone].sum_abs += fabs(val_f);

			if (stats[zone].first) {
			    stats[zone].max = val_f;
			    stats[zone].min = val_f;
			    stats[zone].first = FALSE;
			}
			else {
			    if (val_f > stats[zone].max)
				stats[zone].max = val_f;
			    if (val_f < stats[zone].min)
				stats[zone].min = val_f;
			}
			stats[zone].n++;
		    }
		    stats[zone].size++;
		}
		else if (map_type == DCELL_TYPE) {
		    G3d_getValue(map, x, y, z, &val_d, map_type);
		    if (!G3d_isNullValueNum(&val_d, map_type)) {
			if (param.extended->answer) {
			    if (stats[zone].n >= stats[zone].n_alloc) {
				size_t msize;
				stats[zone].n_alloc += 1000;
				msize = stats[zone].n_alloc * sizeof(DCELL);
				stats[zone].dcell_array =
				    (DCELL *)G_realloc((void *)stats[zone].dcell_array, msize);
				}

			    stats[zone].dcell_array[stats[zone].n] = val_d;
			}

			stats[zone].sum += val_d;
			stats[zone].sumsq += val_d * val_d;
			stats[zone].sum_abs += fabs(val_d);

			if (stats[zone].first) {
			    stats[zone].max = val_d;
			    stats[zone].min = val_d;
			    stats[zone].first = FALSE;
			}
			else {
			    if (val_d > stats[zone].max)
				stats[zone].max = val_d;
			    if (val_d < stats[zone].min)
				stats[zone].min = val_d;
			}
			stats[zone].n++;
		    }
		    stats[zone].size++;
		}
	    }
	}
    }

    /* close maps */
    G3d_closeCell(map);
    if (zone_info.n_zones)
	G3d_closeCell(zmap);

    /* create the output */
    if (param.table->answer)
	print_stats_table(stats);
    else
	print_stats(stats);

    /* release memory */
    free_univar_stat_struct(stats);

    exit(EXIT_SUCCESS);
}
