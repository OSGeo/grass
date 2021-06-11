/*
 * r.univar
 *
 *  Calculates univariate statistics from the non-null cells of a GRASS raster map
 *
 *   Copyright (C) 2004-2006, 2012 by the GRASS Development Team
 *   Author(s): Hamish Bowman, University of Otago, New Zealand
 *              Extended stats: Martin Landa
 *              Zonal stats: Markus Metz
 *
 *      This program is free software under the GNU General Public
 *      License (>=v2). Read the file COPYING that comes with GRASS
 *      for details.
 *
 *   This program is a replacement for the r.univar shell script
 */

#if defined(_OPENMP)
#include <omp.h>
#endif

#include <assert.h>
#include <string.h>
#include <float.h>
#include "globals.h"

param_type param;
zone_type zone_info;

/* ************************************************************************* */
/* Set up the arguments we are expecting ********************************** */
/* ************************************************************************* */
void set_params()
{
    param.inputfile = G_define_standard_option(G_OPT_R_MAPS);

    param.zonefile = G_define_standard_option(G_OPT_R_MAP);
    param.zonefile->key = "zones";
    param.zonefile->required = NO;
    param.zonefile->description =
	_("Raster map used for zoning, must be of type CELL");

    param.output_file = G_define_standard_option(G_OPT_F_OUTPUT);
    param.output_file->required = NO;
    param.output_file->description =
	_("Name for output file (if omitted or \"-\" output to stdout)");
    param.output_file->guisection = _("Output settings");

    param.percentile = G_define_option();
    param.percentile->key = "percentile";
    param.percentile->type = TYPE_DOUBLE;
    param.percentile->required = NO;
    param.percentile->multiple = YES;
    param.percentile->options = "0-100";
    param.percentile->answer = "90";
    param.percentile->description =
	_("Percentile to calculate (requires extended statistics flag)");
    param.percentile->guisection = _("Extended");
    
    param.threads = G_define_option();
    param.threads->key = "nprocs";
    param.threads->type = TYPE_INTEGER;
    param.threads->answer = "1";
    param.threads->options = "1-1000";
    param.threads->required = NO;
    param.threads->description = _("Number of threads which will be used for parallel computing");

    param.separator = G_define_standard_option(G_OPT_F_SEP);
    param.separator->guisection = _("Formatting");

    param.shell_style = G_define_flag();
    param.shell_style->key = 'g';
    param.shell_style->description =
	_("Print the stats in shell script style");
    param.shell_style->guisection = _("Formatting");

    param.extended = G_define_flag();
    param.extended->key = 'e';
    param.extended->description = _("Calculate extended statistics");
    param.extended->guisection = _("Extended");

    param.table = G_define_flag();
    param.table->key = 't';
    param.table->description = _("Table output format instead of standard output format");
    param.table->guisection = _("Formatting");

    param.use_rast_region = G_define_flag();
    param.use_rast_region->key = 'r';
    param.use_rast_region->description = _("Use the native resolution and extent of the raster map, instead of the current region");

    return;
}

static int open_raster(const char *infile);
static univar_stat *univar_stat_with_percentiles(int map_type);
static void process_raster(univar_stat * stats, int fd, int fdz,
			   const struct Cell_head *region);
static void process_raster_threaded(univar_stat *stats, char *fname, 
    char *zone_fname, const struct Cell_head *region, const int threads);

/* *************************************************************** */
/* **** the main functions for r.univar ************************** */
/* *************************************************************** */
int main(int argc, char *argv[])
{
    int rasters;

    struct Cell_head region;
    struct GModule *module;
    univar_stat *stats;
    char **p, *z;
    int fd, fdz, cell_type, min, max;
    struct Range zone_range;
    const char *mapset, *name;

    int threads;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("univariate statistics"));
    G_add_keyword(_("zonal statistics"));

    module->label =
	_("Calculates univariate statistics from the non-null cells of a raster map.");
    module->description =
	_("Statistics include number of cells counted, minimum and maximum cell"
	    " values, range, arithmetic mean, population variance, standard deviation,"
	    " coefficient of variation, and sum.");

    /* Define the different options */
    set_params();

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (param.zonefile->answer && param.use_rast_region->answer) {
    	G_fatal_error(_("zones option and region flag -r are mutually exclusive"));
    }

    name = param.output_file->answer;
    if (name != NULL && strcmp(name, "-") != 0) {
	if (NULL == freopen(name, "w", stdout)) {
	    G_fatal_error(_("Unable to open file <%s> for writing"), name);
	}
    }
    
    /* table field separator */
    zone_info.sep = G_option_to_separator(param.separator);

    zone_info.min = 0.0 / 0.0;	/* set to nan as default */
    zone_info.max = 0.0 / 0.0;	/* set to nan as default */
    zone_info.n_zones = 0;

    fdz = -1;
    
    /* open zoning raster */
    if ((z = param.zonefile->answer)) {
	mapset = G_find_raster2(z, "");

	fdz = open_raster(z);
	
	cell_type = Rast_get_map_type(fdz);
	if (cell_type != CELL_TYPE)
	    G_fatal_error("Zoning raster must be of type CELL");

	if (Rast_read_range(z, mapset, &zone_range) == -1)
	    G_fatal_error("Can not read range for zoning raster");
	Rast_get_range_min_max(&zone_range, &min, &max);
	if (Rast_read_cats(z, mapset, &(zone_info.cats)))
	    G_warning("no category support for zoning raster");

	zone_info.min = min;
	zone_info.max = max;
	zone_info.n_zones = max - min + 1;
    }

    /* count the input rasters given */
    for (p = (char **)param.inputfile->answers, rasters = 0;
	 *p; p++, rasters++) ;

    /* process all input rasters */
    int map_type = param.extended->answer ? -2 : -1;

    stats = ((map_type == -1)
	     ? create_univar_stat_struct(-1, 0)
	     : 0);

    for (p = param.inputfile->answers; *p; p++) {

	/* Check if the native extent and resolution
	   of the input map should be used */
    if(param.use_rast_region->answer) {
        mapset = G_find_raster2(*p, "");
        Rast_get_cellhd(*p, mapset, &region);
        /* Set the computational region */
        Rast_set_window(&region);
    } else {
        G_get_window(&region);
    }

	fd = open_raster(*p);

	if (map_type != -1) {
	    /* NB: map_type must match when doing extended stats */
	    int this_type = Rast_get_map_type(fd);

	    assert(this_type > -1);
	    if (map_type < -1) {
		/* extended stats */
		assert(stats == 0);
		map_type = this_type;
		stats = univar_stat_with_percentiles(map_type);
	    }
	    else if (this_type != map_type) {
		G_fatal_error(_("Raster <%s> type mismatch"), *p);
	    }
	}

    sscanf(param.threads->answer, "%d", &threads);
    if (threads < 1)
    {
      G_warning(_("<%d> is not valid number of threads. Number of threads will be set to <%d>"),
      threads, abs(threads));
      threads = abs(threads);
    }
#if defined(_OPENMP)
    if (!param.extended->answer) {
        omp_set_num_threads(threads);
        process_raster_threaded(stats, *p, z, &region, threads);
    } else {
	    process_raster(stats, fd, fdz, &region);
    }
#else
	process_raster(stats, fd, fdz, &region);
#endif


	/* close input raster */
	Rast_close(fd);
    }

    /* close zoning raster */
    if (z)
	Rast_close(fdz);

    /* create the output */
    if (param.table->answer)
	print_stats_table(stats);
    else
	print_stats(stats);
	
    /* release memory */
    free_univar_stat_struct(stats);

    exit(EXIT_SUCCESS);
}

static int open_raster(const char *infile)
{
    const char *mapset;
    int fd;

    mapset = G_find_raster2(infile, "");
    if (mapset == NULL) {
	G_fatal_error(_("Raster map <%s> not found"), infile);
    }

    fd = Rast_open_old(infile, mapset);
    G_free((void *) mapset);

    return fd;
}

static univar_stat *univar_stat_with_percentiles(int map_type)
{
    univar_stat *stats;
    unsigned int i, j;
    unsigned int n_zones = zone_info.n_zones;

    if (n_zones == 0)
	n_zones = 1;

    i = 0;
    while (param.percentile->answers[i])
	i++;
    stats = create_univar_stat_struct(map_type, i);
    for (i = 0; i < n_zones; i++) {
	for (j = 0; j < stats[i].n_perc; j++) {
	    sscanf(param.percentile->answers[j], "%lf", &(stats[i].perc[j]));
	}
    }

    /* . */
    return stats;
}

static void
process_raster(univar_stat * stats, int fd, int fdz, const struct Cell_head *region)
{
    /* use G_window_rows(), G_window_cols() here? */
    const unsigned int rows = region->rows;
    const unsigned int cols = region->cols;

    const RASTER_MAP_TYPE map_type = Rast_get_map_type(fd);
    const size_t value_sz = Rast_cell_size(map_type);
    unsigned int row;
    void *raster_row;
    CELL *zoneraster_row = NULL;
    int n_zones = zone_info.n_zones;
    
    raster_row = Rast_allocate_buf(map_type);
    if (n_zones)
	zoneraster_row = Rast_allocate_c_buf();

    for (row = 0; row < rows; row++) {
	void *ptr;
	CELL *zptr = NULL;
	unsigned int col;

	Rast_get_row(fd, raster_row, row, map_type);
	if (n_zones) {
	    Rast_get_c_row(fdz, zoneraster_row, row);
	    zptr = zoneraster_row;
	}

	ptr = raster_row;

	for (col = 0; col < cols; col++) {
	    double val;
	    int zone = 0;

	    if (n_zones) {
		/* skip NULL cells in zone map */
		if (Rast_is_c_null_value(zptr)) {
		    ptr = G_incr_void_ptr(ptr, value_sz);
		    zptr++;
		    continue;
		}
		zone = *zptr - zone_info.min;
	    }

	    /* count all including NULL cells in input map */
	    stats[zone].size++;
	    
	    /* can't do stats with NULL cells in input map */
	    if (Rast_is_null_value(ptr, map_type)) {
		ptr = G_incr_void_ptr(ptr, value_sz);
		if (n_zones)
		    zptr++;
		continue;
	    }

	    if (param.extended->answer) {
		/* check allocated memory */
		if (stats[zone].n >= stats[zone].n_alloc) {
		    stats[zone].n_alloc += 1000;
		    size_t msize;
		    switch (map_type) {
			case DCELL_TYPE:
			    msize = stats[zone].n_alloc * sizeof(DCELL);
			    stats[zone].dcell_array =
				(DCELL *)G_realloc((void *)stats[zone].dcell_array, msize);
			    stats[zone].nextp = (void *)&(stats[zone].dcell_array[stats[zone].n]);
			    break;
			case FCELL_TYPE:
			    msize = stats[zone].n_alloc * sizeof(FCELL);
			    stats[zone].fcell_array =
				(FCELL *)G_realloc((void *)stats[zone].fcell_array, msize);
			    stats[zone].nextp = (void *)&(stats[zone].fcell_array[stats[zone].n]);
			    break;
			case CELL_TYPE:
			    msize = stats[zone].n_alloc * sizeof(CELL);
			    stats[zone].cell_array =
				(CELL *)G_realloc((void *)stats[zone].cell_array, msize);
			    stats[zone].nextp = (void *)&(stats[zone].cell_array[stats[zone].n]);
			    break;
			default:
			    break;
		    }
		}
		/* put the value into stats->XXXcell_array */
		memcpy(stats[zone].nextp, ptr, value_sz);
		stats[zone].nextp = G_incr_void_ptr(stats[zone].nextp, value_sz);
	    }

	    val = ((map_type == DCELL_TYPE) ? *((DCELL *) ptr)
			  : (map_type == FCELL_TYPE) ? *((FCELL *) ptr)
			  : *((CELL *) ptr));

	    stats[zone].sum += val;
	    stats[zone].sumsq += val * val;
	    stats[zone].sum_abs += fabs(val);

	    if (stats[zone].first) {
		stats[zone].max = val;
		stats[zone].min = val;
		stats[zone].first = FALSE;
	    }
	    else {
		if (val > stats[zone].max)
		    stats[zone].max = val;
		if (val < stats[zone].min)
		    stats[zone].min = val;
	    }

	    ptr = G_incr_void_ptr(ptr, value_sz);
	    if (n_zones)
		zptr++;
	    stats[zone].n++;
	}
	if (!(param.shell_style->answer))
	    G_percent(row, rows, 2);
    }
    G_free(raster_row);
    if (n_zones)
        G_free(zoneraster_row);
    if (!(param.shell_style->answer))
	G_percent(rows, rows, 2);	/* finish it off */

}

static void
process_raster_threaded(univar_stat * stats, char *fname, char *zone_fname, const struct Cell_head *region, const int threads)
{
    /* use G_window_rows(), G_window_cols() here? */
    const unsigned int rows = region->rows;
    const unsigned int cols = region->cols;
    const int n_zones = zone_info.n_zones;

    const char *mapset;
    const char *zone_mapset;
    mapset = G_find_raster2(fname, "");
    if (n_zones) {
        zone_mapset = G_find_raster2(zone_fname, "");
    }

    /* open file per thread */
    int *fd, *fdz;
    fd = G_malloc(threads * sizeof(int));
    if (n_zones) {
        fdz = G_malloc(threads * sizeof(int));
    }
    for (int i = 0; i < threads; i++) {
        fd[i] = Rast_open_old(fname, mapset);
        if (n_zones)
            fdz[i] = Rast_open_old(zone_fname, zone_mapset);
    }

    const RASTER_MAP_TYPE map_type = Rast_get_map_type(fd[0]);
    const size_t value_sz = Rast_cell_size(map_type);
    void **raster_row;
    CELL **zoneraster_row = NULL;
    raster_row = G_malloc(threads * sizeof(void*));
    if (n_zones) {
        zoneraster_row = G_malloc(threads * sizeof(CELL*));
    }
    for (int i = 0; i < threads; i++) {
        if (NULL == (raster_row[i] = Rast_allocate_buf(map_type))) {
            return;
        }
        if (n_zones != 0 && (NULL == (zoneraster_row[i] = Rast_allocate_c_buf()))) {
            return;
        }
    }

    int n_alloc = n_zones ? n_zones : 1;
    size_t **n = G_malloc(threads * sizeof(size_t*));
    double **sum = G_malloc(threads * sizeof(double*));
    double **sumsq = G_malloc(threads * sizeof(double*));
    double **sum_abs = G_malloc(threads * sizeof(double*));
    double **min = G_malloc(threads * sizeof(double*));
    double **max = G_malloc(threads * sizeof(double*));
    size_t **size = G_malloc(threads * sizeof(double*));

    for (int i = 0; i < threads; i++) {
        n[i] = G_calloc(n_alloc, sizeof(size_t));
        sum[i] = G_calloc(n_alloc, sizeof(double));
        sumsq[i] = G_calloc(n_alloc, sizeof(double));
        sum_abs[i] = G_calloc(n_alloc, sizeof(double));
        size[i] = G_calloc((n_alloc), sizeof(size_t));
        min[i] = G_malloc((n_alloc) * sizeof(double));
        max[i] = G_malloc((n_alloc) * sizeof(double));
        for (int j = 0; j < n_alloc; j++) {
            max[i][j] = DBL_MIN;
            min[i][j] = DBL_MAX;
        }
    }

    omp_lock_t *minmax = G_malloc(n_alloc * sizeof(omp_lock_t));
    for (int i = 0; i < n_alloc; i++) {
        omp_init_lock(&(minmax[i]));
    }

    unsigned int work = 0;
    #pragma omp parallel default(none) \
    shared(stats, fd, fdz, raster_row, zoneraster_row, n, sum, sumsq, sum_abs, min, max, size, region, \
    rows, cols, n_zones, zone_info, param, work, mapset, map_type, value_sz, n_alloc, minmax)
    {
    int id = omp_get_thread_num();
    unsigned int row;

    #pragma omp for schedule(static)
    for (row = 0; row < rows; row++) {
	void *ptr;
	CELL *zptr = NULL;
	unsigned int col;

	Rast_get_row(fd[id], raster_row[id], row, map_type);
	if (n_zones) {
	    Rast_get_c_row(fdz[id], zoneraster_row[id], row);
	    zptr = zoneraster_row[id];
	}

	ptr = raster_row[id];

    for (col = 0; col < cols; col++) {
	    double val;
	    int zone = 0;

	    if (n_zones) {
		/* skip NULL cells in zone map */
		if (Rast_is_c_null_value(zptr)) {
		    ptr = G_incr_void_ptr(ptr, value_sz);
		    zptr++;
		    continue;
		}
		zone = *zptr - zone_info.min;
	    }

	    /* count all including NULL cells in input map */
	    /* stats[zone].size++; */
        size[id][zone]++;
	    
	    /* can't do stats with NULL cells in input map */
	    if (Rast_is_null_value(ptr, map_type)) {
		ptr = G_incr_void_ptr(ptr, value_sz);
		if (n_zones)
		    zptr++;
		continue;
	    }

	    val = ((map_type == DCELL_TYPE) ? *((DCELL *) ptr)
			  : (map_type == FCELL_TYPE) ? *((FCELL *) ptr)
			  : *((CELL *) ptr));

	    sum[id][zone] += val;
	    sumsq[id][zone] += val * val;
	    sum_abs[id][zone] += fabs(val);

		if (val > max[id][zone])
		    max[id][zone] = val;
		if (val < min[id][zone])
		    min[id][zone] = val;

	    ptr = G_incr_void_ptr(ptr, value_sz);
	    if (n_zones)
		zptr++;
        n[id][zone]++;
	} /* end of inner loop */
        if (!(param.shell_style->answer)) {
            #pragma omp atomic update
            work++;
            G_percent(work, rows, 2);
        }
    } /* end of loop region */

    id = omp_get_thread_num();
    for (int i = 0; i < n_alloc; i++) {
        #pragma omp atomic update
        stats[i].n += n[id][i];
        #pragma omp atomic update
        stats[i].size += size[id][i];
        #pragma omp atomic update
        stats[i].sum += sum[id][i];
        #pragma omp atomic update
        stats[i].sumsq += sumsq[id][i];
        #pragma omp atomic update
        stats[i].sum_abs += sum_abs[id][i];

        omp_set_lock(&(minmax[i]));
        if (stats[i].max < max[id][i] || (stats[i].max != stats[i].max && max[id][i] != DBL_MIN)) {
            stats[i].max = max[id][i];
        }
        if (stats[i].min > min[id][i] || (stats[i].min != stats[i].min && min[id][i] != DBL_MAX)) {
            stats[i].min = min[id][i];
        }
        omp_unset_lock(&(minmax[i]));
    }

    Rast_close(fd[id]);
    if (n_zones)
        Rast_close(fdz[id]);
    G_free(n[id]);
    G_free(size[id]);
    G_free(sum[id]);
    G_free(sumsq[id]);
    G_free(sum_abs[id]);
    G_free(max[id]);
    G_free(min[id]);
    G_free(raster_row[id]);
    if (n_zones)
        G_free(zoneraster_row[id]);
} /* end of parallel region */
    if (!(param.shell_style->answer))
	G_percent(rows, rows, 2);	/* finish it off */
    for (int i = 0; i < n_alloc; i++) {
        omp_destroy_lock(&(minmax[i]));
    }
    G_free((void *) mapset);
    if (n_zones)
        G_free((void *) zone_mapset);
    G_free(fd);
    if (n_zones)
        G_free(fdz);
    G_free(n);
    G_free(size);
    G_free(sum);
    G_free(sumsq);
    G_free(sum_abs);
    G_free(max);
    G_free(min);
    G_free(raster_row);
    if (n_zones)
        G_free(zoneraster_row);
}
