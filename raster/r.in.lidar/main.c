/****************************************************************************
 *
 * MODULE:       r.in.Lidar
 *               
 * AUTHOR(S):    Markus Metz
 *               Based on r.in.xyz by Hamish Bowman, Volker Wichmann
 *
 * PURPOSE:      Imports LAS LiDAR point clouds to a raster map using 
 *               aggregate statistics.
 *
 * COPYRIGHT:    (C) 2011 Markus Metz and the The GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include "local_proto.h"

struct node
{
    int next;
    double z;
};

#define	SIZE_INCREMENT 10
int num_nodes = 0;
int max_nodes = 0;
struct node *nodes;


int new_node(void)
{
    int n = num_nodes++;

    if (num_nodes >= max_nodes) {
	max_nodes += SIZE_INCREMENT;
	nodes = G_realloc(nodes, (size_t)max_nodes * sizeof(struct node));
    }

    return n;
}


/* add node to sorted, single linked list 
 * returns id if head has to be saved to index array, otherwise -1 */
int add_node(int head, double z)
{
    int node_id, last_id, newnode_id, head_id;

    head_id = head;
    node_id = head_id;
    last_id = head_id;

    while (node_id != -1 && nodes[node_id].z < z) {
	last_id = node_id;
	node_id = nodes[node_id].next;
    }

    /* end of list, simply append */
    if (node_id == -1) {
	newnode_id = new_node();
	nodes[newnode_id].next = -1;
	nodes[newnode_id].z = z;
	nodes[last_id].next = newnode_id;
	return -1;
    }
    else if (node_id == head_id) {	/* pole position, insert as head */
	newnode_id = new_node();
	nodes[newnode_id].next = head_id;
	head_id = newnode_id;
	nodes[newnode_id].z = z;
	return (head_id);
    }
    else {			/* somewhere in the middle, insert */
	newnode_id = new_node();
	nodes[newnode_id].z = z;
	nodes[newnode_id].next = node_id;
	nodes[last_id].next = newnode_id;
	return -1;
    }
}



int main(int argc, char *argv[])
{
    int out_fd;
    char *infile, *outmap;
    int percent;
    int method = -1;
    int bin_n, bin_min, bin_max, bin_sum, bin_sumsq, bin_index;
    double zrange_min, zrange_max, d_tmp;
    unsigned long estimated_lines;

    RASTER_MAP_TYPE rtype;
    struct History history;
    char title[64];
    void *n_array, *min_array, *max_array, *sum_array, *sumsq_array,
	*index_array;
    void *raster_row, *ptr;
    struct Cell_head region;
    int rows, cols;		/* scan box size */
    int row, col;		/* counters */

    int pass, npasses;
    unsigned long line, line_total;
    unsigned int counter;
    char buff[BUFFSIZE];
    double x, y, z;
    double pass_north, pass_south;
    int arr_row, arr_col;
    unsigned long count, count_total;

    double min = 0.0 / 0.0;	/* init as nan */
    double max = 0.0 / 0.0;	/* init as nan */
    double zscale = 1.0;
    size_t offset, n_offset;
    int n = 0;
    double sum = 0.;
    double sumsq = 0.;
    double variance, mean, skew, sumdev;
    int pth = 0;
    double trim = 0.0;
    double res = 0.0;

    int j, k;
    int head_id, node_id;
    int r_low, r_up;

    struct GModule *module;
    struct Option *input_opt, *output_opt, *percent_opt, *type_opt;
    struct Option *method_opt, *zrange_opt, *zscale_opt;
    struct Option *trim_opt, *pth_opt, *res_opt;
    struct Flag *scan_flag, *shell_style, *over_flag, *extents_flag;

    LASReaderH LAS_reader;
    LASHeaderH LAS_header;
    LASSRSH LAS_srs;
    LASPointH LAS_point;

    struct Key_Value *loc_proj_info = NULL, *loc_proj_units = NULL;
    struct Key_Value *proj_info, *proj_units;
    const char *projstr;
    struct Cell_head cellhd, loc_wind;


    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    G_add_keyword(_("LIDAR"));
    module->description =
	_("Create a raster map from LAS LiDAR points using univariate statistics.");

    input_opt = G_define_standard_option(G_OPT_F_INPUT);
    input_opt->description =
	_("LiDAR LAS input file");

    output_opt = G_define_standard_option(G_OPT_R_OUTPUT);

    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = NO;
    method_opt->description = _("Statistic to use for raster values");
    method_opt->options =
	"n,min,max,range,sum,mean,stddev,variance,coeff_var,median,percentile,skewness,trimmean";
    method_opt->answer = "mean";
    method_opt->guisection = _("Statistic");

    type_opt = G_define_option();
    type_opt->key = "type";
    type_opt->type = TYPE_STRING;
    type_opt->required = NO;
    type_opt->options = "CELL,FCELL,DCELL";
    type_opt->answer = "FCELL";
    type_opt->description = _("Storage type for resultant raster map");

    zrange_opt = G_define_option();
    zrange_opt->key = "zrange";
    zrange_opt->type = TYPE_DOUBLE;
    zrange_opt->required = NO;
    zrange_opt->key_desc = "min,max";
    zrange_opt->description = _("Filter range for z data (min,max)");

    zscale_opt = G_define_option();
    zscale_opt->key = "zscale";
    zscale_opt->type = TYPE_DOUBLE;
    zscale_opt->required = NO;
    zscale_opt->answer = "1.0";
    zscale_opt->description = _("Scale to apply to z data");

    percent_opt = G_define_option();
    percent_opt->key = "percent";
    percent_opt->type = TYPE_INTEGER;
    percent_opt->required = NO;
    percent_opt->answer = "100";
    percent_opt->options = "1-100";
    percent_opt->description = _("Percent of map to keep in memory");

    /* I would prefer to call the following "percentile", but that has too
     * much namespace overlap with the "percent" option above */
    pth_opt = G_define_option();
    pth_opt->key = "pth";
    pth_opt->type = TYPE_INTEGER;
    pth_opt->required = NO;
    pth_opt->options = "1-100";
    pth_opt->description = _("pth percentile of the values");
    pth_opt->guisection = _("Statistic");

    trim_opt = G_define_option();
    trim_opt->key = "trim";
    trim_opt->type = TYPE_DOUBLE;
    trim_opt->required = NO;
    trim_opt->options = "0-50";
    trim_opt->description =
	_("Discard <trim> percent of the smallest and <trim> percent of the largest observations");
    trim_opt->guisection = _("Statistic");

    res_opt = G_define_option();
    res_opt->key = "resolution";
    res_opt->type = TYPE_DOUBLE;
    res_opt->required = NO;
    res_opt->description =
	_("Output raster resolution");

    extents_flag = G_define_flag();
    extents_flag->key = 'e';
    extents_flag->description =
	_("Use input extents instead of region extents");

    over_flag = G_define_flag();
    over_flag->key = 'o';
    over_flag->description =
	_("Override dataset projection (use location's projection)");

    scan_flag = G_define_flag();
    scan_flag->key = 's';
    scan_flag->description = _("Scan data file for extent then exit");
    scan_flag->suppress_required = YES;

    shell_style = G_define_flag();
    shell_style->key = 'g';
    shell_style->description =
	_("In scan mode, print using shell script style");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* parse input values */
    infile = input_opt->answer;
    outmap = output_opt->answer;

    if (shell_style->answer && !scan_flag->answer) {
	scan_flag->answer = 1; /* pointer not int, so set = shell_style->answer ? */
    }

    /* Open LAS file*/
    LAS_reader = LASReader_Create(infile);
    LAS_header = LASReader_GetHeader(LAS_reader);

    if  (LAS_header == NULL) {
	G_fatal_error(_("Input file <%s> is not a LAS LiDAR point cloud"),
	                infile);
    }

    LAS_srs = LASHeader_GetSRS(LAS_header);

    /* Fetch input map projection in GRASS form. */
    proj_info = NULL;
    proj_units = NULL;
    projstr = LASSRS_GetWKT_CompoundOK(LAS_srs);

    if (TRUE) {
	int err = 0;
	char error_msg[8192];

	/* Projection only required for checking so convert non-interactively */
	if (GPJ_wkt_to_grass(&cellhd, &proj_info,
			     &proj_units, projstr, 0) < 0)
	    G_warning(_("Unable to convert input map projection information to "
		       "GRASS format for checking"));
	
	/* Does the projection of the current location match the dataset? */
	/* G_get_window seems to be unreliable if the location has been changed */
	G_get_set_window(&loc_wind);
	/* fetch LOCATION PROJ info */
	if (loc_wind.proj != PROJECTION_XY) {
	    loc_proj_info = G_get_projinfo();
	    loc_proj_units = G_get_projunits();
	}

	if (over_flag->answer) {
	    cellhd.proj = loc_wind.proj;
	    cellhd.zone = loc_wind.zone;
	    G_message(_("Over-riding projection check"));
	}
	else if (loc_wind.proj != cellhd.proj
		 || (err =
		     G_compare_projections(loc_proj_info, loc_proj_units,
					   proj_info, proj_units)) != TRUE) {
	    int i_value;

	    strcpy(error_msg,
		   _("Projection of dataset does not"
		     " appear to match current location.\n\n"));

	    /* TODO: output this info sorted by key: */
	    if (loc_wind.proj != cellhd.proj || err != -2) {
		if (loc_proj_info != NULL) {
		    strcat(error_msg, _("GRASS LOCATION PROJ_INFO is:\n"));
		    for (i_value = 0; i_value < loc_proj_info->nitems;
			 i_value++)
			sprintf(error_msg + strlen(error_msg), "%s: %s\n",
				loc_proj_info->key[i_value],
				loc_proj_info->value[i_value]);
		    strcat(error_msg, "\n");
		}

		if (proj_info != NULL) {
		    strcat(error_msg, _("Import dataset PROJ_INFO is:\n"));
		    for (i_value = 0; i_value < proj_info->nitems; i_value++)
			sprintf(error_msg + strlen(error_msg), "%s: %s\n",
				proj_info->key[i_value],
				proj_info->value[i_value]);
		}
		else {
		    strcat(error_msg, _("Import dataset PROJ_INFO is:\n"));
		    if (cellhd.proj == PROJECTION_XY)
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (unreferenced/unknown)\n",
				cellhd.proj);
		    else if (cellhd.proj == PROJECTION_LL)
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (lat/long)\n",
				cellhd.proj);
		    else if (cellhd.proj == PROJECTION_UTM)
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (UTM), zone = %d\n",
				cellhd.proj, cellhd.zone);
		    else if (cellhd.proj == PROJECTION_SP)
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (State Plane), zone = %d\n",
				cellhd.proj, cellhd.zone);
		    else
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (unknown), zone = %d\n",
				cellhd.proj, cellhd.zone);
		}
	    }
	    else {
		if (loc_proj_units != NULL) {
		    strcat(error_msg, "GRASS LOCATION PROJ_UNITS is:\n");
		    for (i_value = 0; i_value < loc_proj_units->nitems;
			 i_value++)
			sprintf(error_msg + strlen(error_msg), "%s: %s\n",
				loc_proj_units->key[i_value],
				loc_proj_units->value[i_value]);
		    strcat(error_msg, "\n");
		}

		if (proj_units != NULL) {
		    strcat(error_msg, "Import dataset PROJ_UNITS is:\n");
		    for (i_value = 0; i_value < proj_units->nitems; i_value++)
			sprintf(error_msg + strlen(error_msg), "%s: %s\n",
				proj_units->key[i_value],
				proj_units->value[i_value]);
		}
	    }
	    sprintf(error_msg + strlen(error_msg),
		    _("\nYou can use the -o flag to %s to override this projection check.\n"),
		    G_program_name());
	    strcat(error_msg,
		   _("Consider generating a new location with 'location' parameter"
		    " from input data set.\n"));
	    G_fatal_error(error_msg);
	}
	else if (!shell_style->answer) {
	    G_message(_("Projection of input dataset and current location "
			"appear to match"));
	}
    }

    percent = atoi(percent_opt->answer);
    zscale = atof(zscale_opt->answer);

    /* parse zrange */
    if (zrange_opt->answer != NULL) {
	if (zrange_opt->answers[0] == NULL)
	    G_fatal_error(_("Invalid zrange"));

	sscanf(zrange_opt->answers[0], "%lf", &zrange_min);
	sscanf(zrange_opt->answers[1], "%lf", &zrange_max);

	if (zrange_min > zrange_max) {
	    d_tmp = zrange_max;
	    zrange_max = zrange_min;
	    zrange_min = d_tmp;
	}
    }

    /* figure out what maps we need in memory */
    /*  n               n
       min              min
       max              max
       range            min max         max - min
       sum              sum
       mean             sum n           sum/n
       stddev           sum sumsq n     sqrt((sumsq - sum*sum/n)/n)
       variance         sum sumsq n     (sumsq - sum*sum/n)/n
       coeff_var        sum sumsq n     sqrt((sumsq - sum*sum/n)/n) / (sum/n)
       median           n               array index to linked list
       percentile       n               array index to linked list
       skewness         n               array index to linked list
       trimmean         n               array index to linked list
     */
    bin_n = FALSE;
    bin_min = FALSE;
    bin_max = FALSE;
    bin_sum = FALSE;
    bin_sumsq = FALSE;
    bin_index = FALSE;

    n_array = NULL;
    min_array = NULL;
    max_array = NULL;
    sum_array = NULL;
    sumsq_array = NULL;
    index_array = NULL;
    
    if (strcmp(method_opt->answer, "n") == 0) {
	method = METHOD_N;
	bin_n = TRUE;
    }
    if (strcmp(method_opt->answer, "min") == 0) {
	method = METHOD_MIN;
	bin_min = TRUE;
    }
    if (strcmp(method_opt->answer, "max") == 0) {
	method = METHOD_MAX;
	bin_max = TRUE;
    }
    if (strcmp(method_opt->answer, "range") == 0) {
	method = METHOD_RANGE;
	bin_min = TRUE;
	bin_max = TRUE;
    }
    if (strcmp(method_opt->answer, "sum") == 0) {
	method = METHOD_SUM;
	bin_sum = TRUE;
    }
    if (strcmp(method_opt->answer, "mean") == 0) {
	method = METHOD_MEAN;
	bin_sum = TRUE;
	bin_n = TRUE;
    }
    if (strcmp(method_opt->answer, "stddev") == 0) {
	method = METHOD_STDDEV;
	bin_sum = TRUE;
	bin_sumsq = TRUE;
	bin_n = TRUE;
    }
    if (strcmp(method_opt->answer, "variance") == 0) {
	method = METHOD_VARIANCE;
	bin_sum = TRUE;
	bin_sumsq = TRUE;
	bin_n = TRUE;
    }
    if (strcmp(method_opt->answer, "coeff_var") == 0) {
	method = METHOD_COEFF_VAR;
	bin_sum = TRUE;
	bin_sumsq = TRUE;
	bin_n = TRUE;
    }
    if (strcmp(method_opt->answer, "median") == 0) {
	method = METHOD_MEDIAN;
	bin_index = TRUE;
    }
    if (strcmp(method_opt->answer, "percentile") == 0) {
	if (pth_opt->answer != NULL)
	    pth = atoi(pth_opt->answer);
	else
	    G_fatal_error(_("Unable to calculate percentile without the pth option specified!"));
	method = METHOD_PERCENTILE;
	bin_index = TRUE;
    }
    if (strcmp(method_opt->answer, "skewness") == 0) {
	method = METHOD_SKEWNESS;
	bin_index = TRUE;
    }
    if (strcmp(method_opt->answer, "trimmean") == 0) {
	if (trim_opt->answer != NULL)
	    trim = atof(trim_opt->answer) / 100.0;
	else
	    G_fatal_error(_("Unable to calculate trimmed mean without the trim option specified!"));
	method = METHOD_TRIMMEAN;
	bin_index = TRUE;
    }

    if (strcmp("CELL", type_opt->answer) == 0)
	rtype = CELL_TYPE;
    else if (strcmp("DCELL", type_opt->answer) == 0)
	rtype = DCELL_TYPE;
    else
	rtype = FCELL_TYPE;

    if (method == METHOD_N)
	rtype = CELL_TYPE;


    Rast_get_window(&region);


    if (scan_flag->answer || extents_flag->answer) {
	if (zrange_opt->answer)
	    G_warning(_("zrange will not be taken into account during scan"));

	scan_bounds(LAS_reader, shell_style->answer, extents_flag->answer,
	            zscale, &region);

	if (!extents_flag->answer) {
	    LASHeader_Destroy(LAS_header);
	    LASReader_Destroy(LAS_reader);

	    exit(EXIT_SUCCESS);
	}
    }
    
    if (res_opt->answer) {
	/* align to resolution */
	res = atof(res_opt->answer);

	if (!G_scan_resolution(res_opt->answer, &res, region.proj))
	    G_fatal_error(_("Invalid input <%s=%s>"), res_opt->key, res_opt->answer);

	if (res <= 0)
	    G_fatal_error(_("Option '%s' must be > 0.0"), res_opt->key);
	
	region.ns_res = region.ew_res = res;

	region.north = ceil(region.north / res) * res;
	region.south = floor(region.south / res) * res;
	region.east = ceil(region.east / res) * res;
	region.west = floor(region.west / res) * res;

	G_adjust_Cell_head(&region, 0, 0);
    }
    else if (extents_flag->answer) {
	/* align to current region */
	Rast_align_window(&region, &loc_wind);
    }
    Rast_set_output_window(&region);

    rows = (int)(region.rows * (percent / 100.0));
    cols = region.cols;

    G_debug(2, "region.n=%f  region.s=%f  region.ns_res=%f", region.north,
	    region.south, region.ns_res);
    G_debug(2, "region.rows=%d  [box_rows=%d]  region.cols=%d", region.rows,
	    rows, region.cols);

    npasses = (int)ceil(1.0 * region.rows / rows);

    if (!scan_flag->answer) {
	/* check if rows * (cols + 1) go into a size_t */
	if (sizeof(size_t) < 8) {
	    double dsize = rows * (cols + 1);
	    
	    if (dsize != (size_t)rows * (cols + 1))
		G_fatal_error(_("Unable to process the hole map at once. "
		                "Please set the %s option to some value lower than 100."),
				percent_opt->key);
	}
	/* allocate memory (test for enough before we start) */
	if (bin_n)
	    n_array = G_calloc((size_t)rows * (cols + 1), Rast_cell_size(CELL_TYPE));
	if (bin_min)
	    min_array = G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
	if (bin_max)
	    max_array = G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
	if (bin_sum)
	    sum_array = G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
	if (bin_sumsq)
	    sumsq_array = G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
	if (bin_index)
	    index_array =
		G_calloc((size_t)rows * (cols + 1), Rast_cell_size(CELL_TYPE));

	/* and then free it again */
	if (bin_n)
	    G_free(n_array);
	if (bin_min)
	    G_free(min_array);
	if (bin_max)
	    G_free(max_array);
	if (bin_sum)
	    G_free(sum_array);
	if (bin_sumsq)
	    G_free(sumsq_array);
	if (bin_index)
	    G_free(index_array);

	/** end memory test **/
    }


    /* open output map */
    out_fd = Rast_open_new(outmap, rtype);

    estimated_lines = LASHeader_GetPointRecordsCount(LAS_header);

    /* allocate memory for a single row of output data */
    raster_row = Rast_allocate_output_buf(rtype);

    G_message(_("Reading data ..."));

    count_total = line_total = 0;

    /* init northern border */
    pass_south = region.north;

    /* main binning loop(s) */
    for (pass = 1; pass <= npasses; pass++) {
	LASError LAS_error;

	if (npasses > 1)
	    G_message(_("Pass #%d (of %d) ..."), pass, npasses);

	LAS_error = LASReader_Seek(LAS_reader, 0);
	
	if (LAS_error != LE_None)
	    G_fatal_error(_("Could not rewind input file"));

	/* figure out segmentation */
	pass_north = pass_south;  /* exact copy to avoid fp errors */
	pass_south = region.north - pass * rows * region.ns_res;
	if (pass == npasses) {
	    rows = region.rows - (pass - 1) * rows;
	    pass_south = region.south; /* exact copy to avoid fp errors */
	}

	G_debug(2, "pass=%d/%d  pass_n=%f  pass_s=%f  rows=%d",
		pass, npasses, pass_north, pass_south, rows);


	if (bin_n) {
	    G_debug(2, "allocating n_array");
	    n_array = G_calloc((size_t)rows * (cols + 1), Rast_cell_size(CELL_TYPE));
	    blank_array(n_array, rows, cols, CELL_TYPE, 0);
	}
	if (bin_min) {
	    G_debug(2, "allocating min_array");
	    min_array = G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
	    blank_array(min_array, rows, cols, rtype, -1);	/* fill with NULLs */
	}
	if (bin_max) {
	    G_debug(2, "allocating max_array");
	    max_array = G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
	    blank_array(max_array, rows, cols, rtype, -1);	/* fill with NULLs */
	}
	if (bin_sum) {
	    G_debug(2, "allocating sum_array");
	    sum_array = G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
	    blank_array(sum_array, rows, cols, rtype, 0);
	}
	if (bin_sumsq) {
	    G_debug(2, "allocating sumsq_array");
	    sumsq_array = G_calloc((size_t)rows * (cols + 1), Rast_cell_size(rtype));
	    blank_array(sumsq_array, rows, cols, rtype, 0);
	}
	if (bin_index) {
	    G_debug(2, "allocating index_array");
	    index_array =
		G_calloc((size_t)rows * (cols + 1), Rast_cell_size(CELL_TYPE));
	    blank_array(index_array, rows, cols, CELL_TYPE, -1);	/* fill with NULLs */
	}

	line = 0;
	count = 0;
	counter = 0;
	G_percent_reset();

	while ((LAS_point = LASReader_GetNextPoint(LAS_reader)) != NULL) {
	    line++;
	    counter++;

	    if (counter == 10000) {	/* speed */
		if (line < estimated_lines)
		    G_percent(line, estimated_lines, 3);
		counter = 0;
	    }

	    if (!LASPoint_IsValid(LAS_point)) {
		continue;
	    }

	    x = LASPoint_GetX(LAS_point);
	    y = LASPoint_GetY(LAS_point);
	    z = LASPoint_GetZ(LAS_point);

	    if (y <= pass_south || y > pass_north) {
		continue;
	    }
	    if (x < region.west || x >= region.east) {
		continue;
	    }

	    z = z * zscale;

	    if (zrange_opt->answer) {
		if (z < zrange_min || z > zrange_max) {
		    continue;
		}
	    }

	    count++;
	    /*          G_debug(5, "x: %f, y: %f, z: %f", x, y, z); */

	    /* find the bin in the current array box */
	    arr_row = (int)((pass_north - y) / region.ns_res);
	    arr_col = (int)((x - region.west) / region.ew_res);

	    if (bin_n)
		update_n(n_array, cols, arr_row, arr_col);
	    if (bin_min)
		update_min(min_array, cols, arr_row, arr_col, rtype, z);
	    if (bin_max)
		update_max(max_array, cols, arr_row, arr_col, rtype, z);
	    if (bin_sum)
		update_sum(sum_array, cols, arr_row, arr_col, rtype, z);
	    if (bin_sumsq)
		update_sumsq(sumsq_array, cols, arr_row, arr_col, rtype, z);
	    if (bin_index) {
		ptr = index_array;
		ptr =
		    G_incr_void_ptr(ptr,
				    ((arr_row * cols) +
				     arr_col) * Rast_cell_size(CELL_TYPE));

		if (Rast_is_null_value(ptr, CELL_TYPE)) {	/* first node */
		    head_id = new_node();
		    nodes[head_id].next = -1;
		    nodes[head_id].z = z;
		    Rast_set_c_value(ptr, head_id, CELL_TYPE);	/* store index to head */
		}
		else {		/* head is already there */

		    head_id = Rast_get_c_value(ptr, CELL_TYPE);	/* get index to head */
		    head_id = add_node(head_id, z);
		    if (head_id != -1)
			Rast_set_c_value(ptr, head_id, CELL_TYPE);	/* store index to head */
		}
	    }
	}			/* while !EOF */

	G_percent(1, 1, 1);	/* flush */
	G_debug(2, "pass %d finished, %lu coordinates in box", pass, count);
	count_total += count;
	line_total += line;

	/* calc stats and output */
	G_message(_("Writing to map ..."));
	for (row = 0; row < rows; row++) {

	    switch (method) {
	    case METHOD_N:	/* n is a straight copy */
		Rast_raster_cpy(raster_row,
			     n_array +
			     (row * cols * Rast_cell_size(CELL_TYPE)), cols,
			     CELL_TYPE);
		break;

	    case METHOD_MIN:
		Rast_raster_cpy(raster_row,
			     min_array + (row * cols * Rast_cell_size(rtype)),
			     cols, rtype);
		break;

	    case METHOD_MAX:
		Rast_raster_cpy(raster_row,
			     max_array + (row * cols * Rast_cell_size(rtype)),
			     cols, rtype);
		break;

	    case METHOD_SUM:
		Rast_raster_cpy(raster_row,
			     sum_array + (row * cols * Rast_cell_size(rtype)),
			     cols, rtype);
		break;

	    case METHOD_RANGE:	/* (max-min) */
		ptr = raster_row;
		for (col = 0; col < cols; col++) {
		    offset = (row * cols + col) * Rast_cell_size(rtype);
		    min = Rast_get_d_value(min_array + offset, rtype);
		    max = Rast_get_d_value(max_array + offset, rtype);
		    Rast_set_d_value(ptr, max - min, rtype);
		    ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
		}
		break;

	    case METHOD_MEAN:	/* (sum / n) */
		ptr = raster_row;
		for (col = 0; col < cols; col++) {
		    offset = (row * cols + col) * Rast_cell_size(rtype);
		    n_offset = (row * cols + col) * Rast_cell_size(CELL_TYPE);
		    n = Rast_get_c_value(n_array + n_offset, CELL_TYPE);
		    sum = Rast_get_d_value(sum_array + offset, rtype);

		    if (n == 0)
			Rast_set_null_value(ptr, 1, rtype);
		    else
			Rast_set_d_value(ptr, (sum / n), rtype);

		    ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
		}
		break;

	    case METHOD_STDDEV:	/*  sqrt(variance)        */
	    case METHOD_VARIANCE:	/*  (sumsq - sum*sum/n)/n */
	    case METHOD_COEFF_VAR:	/*  100 * stdev / mean    */
		ptr = raster_row;
		for (col = 0; col < cols; col++) {
		    offset = (row * cols + col) * Rast_cell_size(rtype);
		    n_offset = (row * cols + col) * Rast_cell_size(CELL_TYPE);
		    n = Rast_get_c_value(n_array + n_offset, CELL_TYPE);
		    sum = Rast_get_d_value(sum_array + offset, rtype);
		    sumsq = Rast_get_d_value(sumsq_array + offset, rtype);

		    if (n == 0)
			Rast_set_null_value(ptr, 1, rtype);
		    else if (n == 1)
			Rast_set_d_value(ptr, 0.0, rtype);
		    else {
			variance = (sumsq - sum * sum / n) / n;
			if (variance < GRASS_EPSILON)
			    variance = 0.0;

			/* nan test */
			if (variance != variance)
			    Rast_set_null_value(ptr, 1, rtype);
			else {

			    if (method == METHOD_STDDEV)
				variance = sqrt(variance);

			    else if (method == METHOD_COEFF_VAR)
				variance = 100 * sqrt(variance) / (sum / n);

			    /* nan test */
			    if (variance != variance)
				variance = 0.0; /* OK for n > 0 ?*/

			    Rast_set_d_value(ptr, variance, rtype);
			}

		    }
		    ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
		}
		break;
	    case METHOD_MEDIAN:	/* median, if only one point in cell we will use that */
		ptr = raster_row;
		for (col = 0; col < cols; col++) {
		    n_offset = (row * cols + col) * Rast_cell_size(CELL_TYPE);
		    if (Rast_is_null_value(index_array + n_offset, CELL_TYPE))	/* no points in cell */
			Rast_set_null_value(ptr, 1, rtype);
		    else {	/* one or more points in cell */

			head_id =
			    Rast_get_c_value(index_array + n_offset,
						 CELL_TYPE);
			node_id = head_id;

			n = 0;

			while (node_id != -1) {	/* count number of points in cell */
			    n++;
			    node_id = nodes[node_id].next;
			}

			if (n == 1)	/* only one point, use that */
			    Rast_set_d_value(ptr, nodes[head_id].z,
						 rtype);
			else if (n % 2 != 0) {	/* odd number of points: median_i = (n + 1) / 2 */
			    n = (n + 1) / 2;
			    node_id = head_id;
			    for (j = 1; j < n; j++)	/* get "median element" */
				node_id = nodes[node_id].next;

			    Rast_set_d_value(ptr, nodes[node_id].z,
						 rtype);
			}
			else {	/* even number of points: median = (val_below + val_above) / 2 */

			    z = (n + 1) / 2.0;
			    n = floor(z);
			    node_id = head_id;
			    for (j = 1; j < n; j++)	/* get element "below" */
				node_id = nodes[node_id].next;

			    z = (nodes[node_id].z +
				 nodes[nodes[node_id].next].z) / 2;
			    Rast_set_d_value(ptr, z, rtype);
			}
		    }
		    ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
		}
		break;
	    case METHOD_PERCENTILE:	/* rank = (pth*(n+1))/100; interpolate linearly */
		ptr = raster_row;
		for (col = 0; col < cols; col++) {
		    n_offset = (row * cols + col) * Rast_cell_size(CELL_TYPE);
		    if (Rast_is_null_value(index_array + n_offset, CELL_TYPE))	/* no points in cell */
			Rast_set_null_value(ptr, 1, rtype);
		    else {
			head_id =
			    Rast_get_c_value(index_array + n_offset,
						 CELL_TYPE);
			node_id = head_id;
			n = 0;

			while (node_id != -1) {	/* count number of points in cell */
			    n++;
			    node_id = nodes[node_id].next;
			}

			z = (pth * (n + 1)) / 100.0;
			r_low = floor(z);	/* lower rank */
			if (r_low < 1)
			    r_low = 1;
			else if (r_low > n)
			    r_low = n;

			r_up = ceil(z);	/* upper rank */
			if (r_up > n)
			    r_up = n;

			node_id = head_id;
			for (j = 1; j < r_low; j++)	/* search lower value */
			    node_id = nodes[node_id].next;

			z = nodes[node_id].z;	/* save lower value */
			node_id = head_id;
			for (j = 1; j < r_up; j++)	/* search upper value */
			    node_id = nodes[node_id].next;

			z = (z + nodes[node_id].z) / 2;
			Rast_set_d_value(ptr, z, rtype);
		    }
		    ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
		}
		break;
	    case METHOD_SKEWNESS:	/* skewness = sum(xi-mean)^3/(N-1)*s^3 */
		ptr = raster_row;
		for (col = 0; col < cols; col++) {
		    n_offset = (row * cols + col) * Rast_cell_size(CELL_TYPE);
		    if (Rast_is_null_value(index_array + n_offset, CELL_TYPE))	/* no points in cell */
			Rast_set_null_value(ptr, 1, rtype);
		    else {
			head_id =
			    Rast_get_c_value(index_array + n_offset,
						 CELL_TYPE);
			node_id = head_id;

			n = 0;	/* count */
			sum = 0.0;	/* sum */
			sumsq = 0.0;	/* sum of squares */
			sumdev = 0.0;	/* sum of (xi - mean)^3 */
			skew = 0.0;	/* skewness */

			while (node_id != -1) {
			    z = nodes[node_id].z;
			    n++;
			    sum += z;
			    sumsq += (z * z);
			    node_id = nodes[node_id].next;
			}

			if (n > 1) {	/* if n == 1, skew is "0.0" */
			    mean = sum / n;
			    node_id = head_id;
			    while (node_id != -1) {
				z = nodes[node_id].z;
				sumdev += pow((z - mean), 3);
				node_id = nodes[node_id].next;
			    }

			    variance = (sumsq - sum * sum / n) / n;
			    if (variance < GRASS_EPSILON)
				skew = 0.0;
			    else
				skew =
				    sumdev / ((n - 1) *
					      pow(sqrt(variance), 3));
			}
			Rast_set_d_value(ptr, skew, rtype);
		    }
		    ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
		}
		break;
	    case METHOD_TRIMMEAN:
		ptr = raster_row;
		for (col = 0; col < cols; col++) {
		    n_offset = (row * cols + col) * Rast_cell_size(CELL_TYPE);
		    if (Rast_is_null_value(index_array + n_offset, CELL_TYPE))	/* no points in cell */
			Rast_set_null_value(ptr, 1, rtype);
		    else {
			head_id =
			    Rast_get_c_value(index_array + n_offset,
						 CELL_TYPE);

			node_id = head_id;
			n = 0;
			while (node_id != -1) {	/* count number of points in cell */
			    n++;
			    node_id = nodes[node_id].next;
			}

			if (1 == n)
			    mean = nodes[head_id].z;
			else {
			    k = floor(trim * n + 0.5);	/* number of ranks to discard on each tail */

			    if (k > 0 && (n - 2 * k) > 0) {	/* enough elements to discard */
				node_id = head_id;
				for (j = 0; j < k; j++)	/* move to first rank to consider */
				    node_id = nodes[node_id].next;

				j = k + 1;
				k = n - k;
				n = 0;
				sum = 0.0;

				while (j <= k) {	/* get values in interval */
				    n++;
				    sum += nodes[node_id].z;
				    node_id = nodes[node_id].next;
				    j++;
				}
			    }
			    else {
				node_id = head_id;
				n = 0;
				sum = 0.0;
				while (node_id != -1) {
				    n++;
				    sum += nodes[node_id].z;
				    node_id = nodes[node_id].next;
				}
			    }
			    mean = sum / n;
			}
			Rast_set_d_value(ptr, mean, rtype);
		    }
		    ptr = G_incr_void_ptr(ptr, Rast_cell_size(rtype));
		}
		break;

	    default:
		G_fatal_error("?");
	    }

	    /* write out line of raster data */
	    Rast_put_row(out_fd, raster_row, rtype);
	}

	/* free memory */
	if (bin_n)
	    G_free(n_array);
	if (bin_min)
	    G_free(min_array);
	if (bin_max)
	    G_free(max_array);
	if (bin_sum)
	    G_free(sum_array);
	if (bin_sumsq)
	    G_free(sumsq_array);
	if (bin_index) {
	    G_free(index_array);
	    G_free(nodes);
	    num_nodes = 0;
	    max_nodes = 0;
	    nodes = NULL;
	}

    }				/* passes loop */

    G_percent(1, 1, 1);		/* flush */
    G_free(raster_row);

    /* close input LAS file */
    LASHeader_Destroy(LAS_header);
    LASReader_Destroy(LAS_reader);

    /* close raster file & write history */
    Rast_close(out_fd);

    sprintf(title, "Raw x,y,z data binned into a raster grid by cell %s",
	    method_opt->answer);
    Rast_put_cell_title(outmap, title);

    Rast_short_history(outmap, "raster", &history);
    Rast_command_history(&history);
    Rast_set_history(&history, HIST_DATSRC_1, infile);
    Rast_write_history(outmap, &history);


    sprintf(buff, _("%lu points found in region."), count_total);
    G_done_msg(buff);
    G_debug(1, "Processed %lu points.", line_total);

    exit(EXIT_SUCCESS);

}



int scan_bounds(LASReaderH LAS_reader, int shell_style, int extents,
		double zscale, struct Cell_head *region)
{
    unsigned long line;
    int first;
    double min_x, max_x, min_y, max_y, min_z, max_z;
    double x, y, z;
    LASPointH LAS_point;

    line = 0;
    first = TRUE;
    
    /* init to nan in case no points are found */
    min_x = max_x = min_y = max_y = min_z = max_z = 0.0 / 0.0;

    G_verbose_message(_("Scanning data ..."));
    
    LASReader_Seek(LAS_reader, 0);

    while ((LAS_point = LASReader_GetNextPoint(LAS_reader)) != NULL) {
	line++;

	if (!LASPoint_IsValid(LAS_point)) {
	    continue;
	}

	x = LASPoint_GetX(LAS_point);
	y = LASPoint_GetY(LAS_point);
	z = LASPoint_GetZ(LAS_point);

	if (first) {
	    min_x = x;
	    max_x = x;
	    min_y = y;
	    max_y = y;
	    min_z = z;
	    max_z = z;
	    first = FALSE;
	}
	else {
	    if (x < min_x)
		min_x = x;
	    if (x > max_x)
		max_x = x;
	    if (y < min_y)
		min_y = y;
	    if (y > max_y)
		max_y = y;
	    if (z < min_z)
		min_z = z;
	    if (z > max_z)
		max_z = z;
	}
    }

    if (!extents) {
	if (!shell_style) {
	    fprintf(stderr, _("Range:     min         max\n"));
	    fprintf(stdout, "x: %11f %11f\n", min_x, max_x);
	    fprintf(stdout, "y: %11f %11f\n", min_y, max_y);
	    fprintf(stdout, "z: %11f %11f\n", min_z * zscale, max_z * zscale);
	}
	else
	    fprintf(stdout, "n=%f s=%f e=%f w=%f b=%f t=%f\n",
		    max_y, min_y, max_x, min_x, min_z * zscale, max_z * zscale);

	G_debug(1, "Processed %lu points.", line);
	G_debug(1, "region template: g.region n=%f s=%f e=%f w=%f",
		max_y, min_y, max_x, min_x);
    }
    else {
	region->east = max_x;
	region->west = min_x;
	region->north = max_y;
	region->south = min_y;
    }

    return 0;
}
