
/****************************************************************************
 * 
 * MODULE:       r3.neighbors
 *              
 * AUTHOR(S):    Original author 
 *               Soeren Gebbert soerengebbert <at> googlemail <dot> co
 *               with code from r.series and r.neighbors for parameter menu handling
 * 
 * PURPOSE:      Makes each voxel value a function of the values assigned to the voxels 
 *               around it, and stores new voxel values in an output 3D raster map
 *
 * COPYRIGHT:    (C) 2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/raster3d.h>
#include <grass/stats.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

int nx, ny, nz;			/* Number of cells in x, y and z direction */
int x_dist, y_dist, z_dist;	/* Distance of cells from the center
				   to the edge of the moving window */
int x_size, y_size, z_size;	/* The size of the moving window in x,
				   y and z direction */
int size;			/* The maximum size of the value buffer */

struct menu
{
    stat_func *method;		/* routine to compute new value */
    char *name;			/* method name */
    char *text;			/* menu display - full description */
} menu[] = {
    {
    c_ave, "average", "average value"}, {
    c_median, "median", "median value"}, {
    c_mode, "mode", "most frequently occuring value"}, {
    c_min, "minimum", "lowest value"}, {
    c_max, "maximum", "highest value"}, {
    c_range, "range", "range value"}, {
    c_stddev, "stddev", "standard deviation"}, {
    c_sum, "sum", "sum of values"}, {
    c_count, "count", "count of non-NULL values"}, {
    c_var, "variance", "statistical variance"}, {
    c_divr, "diversity", "number of different values"}, {
    c_intr, "interspersion", "number of values different than center value"},
    {
    c_quart1, "quart1", "first quartile"}, {
    c_quart3, "quart3", "third quartile"}, {
    c_perc90, "perc90", "ninetieth percentile"}, {
    c_quant, "quantile", "arbitrary quantile"}, {
    NULL, NULL, NULL}
};

/* ************************************************************************* */

static char *build_method_list(void)
{
    char *buf = G_malloc(1024);
    char *p = buf;
    int i;

    for (i = 0; menu[i].name; i++) {
	char *q;

	if (i)
	    *p++ = ',';
	for (q = menu[i].name; *q; p++, q++)
	    *p = *q;
    }
    *p = '\0';

    return buf;
}

/* ************************************************************************* */

static int find_method(const char *method_name)
{
    int i;

    for (i = 0; menu[i].name; i++)
	if (strcmp(menu[i].name, method_name) == 0)
	    return i;

    G_fatal_error(_("Unknown method <%s>"), method_name);

    return -1;
}

/* ************************************************************************* */

typedef struct
{
    struct Option *input, *output, *window, *method, *quantile;
} paramType;

paramType param;

static void set_params()
{
    param.input = G_define_standard_option(G_OPT_R3_INPUT);

    param.output = G_define_standard_option(G_OPT_R3_OUTPUT);

    param.method = G_define_option();
    param.method->key = "method";
    param.method->type = TYPE_STRING;
    param.method->required = YES;
    param.method->options = build_method_list();
    param.method->description = _("Aggregate operation");
    param.method->multiple = NO;

    param.quantile = G_define_option();
    param.quantile->key = "quantile";
    param.quantile->type = TYPE_DOUBLE;
    param.quantile->required = NO;
    param.quantile->description =
	_("Quantile to calculate for method=quantile");
    param.quantile->options = "0.0-1.0";
    param.quantile->multiple = NO;

    param.window = G_define_option();
    param.window->key = "window";
    param.window->type = TYPE_INTEGER;
    param.window->required = YES;
    param.window->key_desc = "x,y,z";
    param.window->description =
	_("The size of the window in x, y and z direction,"
	  " values must be odd integer numbers, eg: 3,3,3");
}

/* ************************************************************************* */

static int gather_values(RASTER3D_Map * map, DCELL * buff, int x,
			 int y, int z)
{
    int i, j, k, l;
    DCELL value;

    int start_z = z - z_dist;
    int start_y = y - y_dist;
    int start_x = x - x_dist;
    int end_z = start_z + z_size;
    int end_y = start_y + y_size;
    int end_x = start_x + x_size;

    if (start_z < 0)
	start_z = 0;

    if (start_y < 0)
	start_y = 0;

    if (start_x < 0)
	start_x = 0;

    if (end_z > nz)
	end_z = nz;

    if (end_y > ny)
	end_y = ny;

    if (end_x > nx)
	end_x = nx;

    l = 0;

    for (i = start_z; i < end_z; i++) {
	for (j = start_y; j < end_y; j++) {
	    for (k = start_x; k < end_x; k++) {
		value = (DCELL) Rast3d_get_double(map, k, j, i);

		if (Rast_is_d_null_value(&value))
		    continue;

		buff[l] = value;
		l++;
	    }
	}
    }
    return l;
}

/* ************************************************************************* */

int main(int argc, char **argv)
{
    RASTER3D_Map *input;
    RASTER3D_Map *output;
    RASTER3D_Region region;
    struct GModule *module;
    stat_func *method_fn;
    double quantile;
    int x, y, z;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("neighbor"));
    G_add_keyword(_("aggregation"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("filter"));
    module->description =
	_("Makes each voxel value a "
	  "function of the values assigned to the voxels "
	  "around it, and stores new voxel values in an output 3D raster map");

    /* Get parameters from user */
    set_params();

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (NULL == G_find_raster3d(param.input->answer, ""))
	Rast3d_fatal_error(_("3D raster map <%s> not found"),
			   param.input->answer);

    Rast3d_init_defaults();
    Rast3d_get_window(&region);

    nx = region.cols;
    ny = region.rows;
    nz = region.depths;

    /* Size fo the moving window */
    x_size = atoi(param.window->answers[0]);
    y_size = atoi(param.window->answers[1]);
    z_size = atoi(param.window->answers[2]);

    /* Distances in all directions */
    x_dist = x_size / 2;
    y_dist = y_size / 2;
    z_dist = z_size / 2;

    /* Maximum size of the buffer */
    size = x_size * y_size * z_size;

    /* Set the computation method */
    method_fn = menu[find_method(param.method->answer)].method;

    if (param.quantile->answer)
	quantile = atof(param.quantile->answer);
    else
	quantile = 0.0;

    input = Rast3d_open_cell_old(param.input->answer,
				 G_find_raster3d(param.input->answer, ""),
				 &region, RASTER3D_TILE_SAME_AS_FILE,
				 RASTER3D_USE_CACHE_DEFAULT);

    if (input == NULL)
	Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
			   param.input->answer);

    output =
	Rast3d_open_new_opt_tile_size(param.output->answer,
				      RASTER3D_USE_CACHE_X, &region,
				      DCELL_TYPE, 32);

    if (output == NULL)
	Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
			   param.output->answer);

    Rast3d_min_unlocked(output, RASTER3D_USE_CACHE_X);
    Rast3d_autolock_on(output);
    Rast3d_unlock_all(output);

    DCELL *buff = NULL, value;

    buff = (DCELL *) calloc(size, sizeof(DCELL));

    if (buff == NULL)
	Rast3d_fatal_error(_("Unable to allocate buffer"));

    for (z = 0; z < nz; z++) {
	for (y = 0; y < ny; y++) {
	    for (x = 0; x < nx; x++) {
		/* Gather values in moving window */
		int num = gather_values(input, buff, x, y, z);

		/* Compute the resulting value */
		if (num > 0)
		    (*method_fn) (&value, buff, num, &quantile);
		else
		    Rast_set_d_null_value(&value, 1);
		/* Write the value */
		Rast3d_put_double(output, x, y, z, value);
	    }
	}
    }

    free(buff);

    if (!Rast3d_flush_all_tiles(output))
	G_fatal_error(_("Error flushing tiles"));

    Rast3d_autolock_off(output);
    Rast3d_unlock_all(output);

    Rast3d_close(input);
    Rast3d_close(output);

    return 0;
}
