 /****************************************************************************
 *
 * MODULE:       r.in.Lidar
 *               
 * AUTHOR(S):    Markus Metz
 *               Vaclav Petras (base raster addition and refactoring)
 *               Based on r.in.xyz by Hamish Bowman, Volker Wichmann
 *
 * PURPOSE:      Imports LAS LiDAR point clouds to a raster map using 
 *               aggregate statistics.
 *
 * COPYRIGHT:    (C) 2011-2015 Markus Metz and the The GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/segment.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include <liblas/capi/liblas.h>

#include "local_proto.h"
#include "rast_segment.h"

struct node
{
    int next;
    double z;
};

#define	SIZE_INCREMENT 10
int num_nodes = 0;
int max_nodes = 0;
struct node *nodes;

#define LAS_ALL 0
#define LAS_FIRST 1
#define LAS_LAST 2
#define LAS_MID 3

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
    int out_fd, base_raster;
    char *infile, *outmap;
    int percent;
    int method = -1;
    int bin_n, bin_min, bin_max, bin_sum, bin_sumsq, bin_index;
    double zrange_min, zrange_max, d_tmp;
    unsigned long estimated_lines;

    RASTER_MAP_TYPE rtype, base_raster_data_type;
    struct History history;
    char title[64];
    SEGMENT base_segment;
    void *n_array, *min_array, *max_array, *sum_array, *sumsq_array,
        *index_array, *base_array;
    void *raster_row, *ptr;
    struct Cell_head region;
    struct Cell_head input_region;
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
    int skipme, i;
    int point_class;

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
    struct Option *input_opt, *output_opt, *percent_opt, *type_opt, *filter_opt, *class_opt;
    struct Option *method_opt, *base_raster_opt, *zrange_opt, *zscale_opt;
    struct Option *trim_opt, *pth_opt, *res_opt;
    struct Flag *print_flag, *scan_flag, *shell_style, *over_flag, *extents_flag, *intens_flag;
    struct Flag *base_rast_res_flag;

    /* LAS */
    LASReaderH LAS_reader;
    LASHeaderH LAS_header;
    LASSRSH LAS_srs;
    LASPointH LAS_point;
    int return_filter;

    const char *projstr;
    struct Cell_head cellhd, loc_wind;

    unsigned int n_filtered;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    G_add_keyword(_("LIDAR"));
    module->description =
	_("Creates a raster map from LAS LiDAR points using univariate statistics.");

    input_opt = G_define_standard_option(G_OPT_F_INPUT);
    input_opt->label = _("LAS input file");
    input_opt->description = _("LiDAR input files in LAS format (*.las or *.laz)");

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

    base_raster_opt = G_define_standard_option(G_OPT_R_INPUT);
    base_raster_opt->key = "base_raster";
    base_raster_opt->required = NO;
    base_raster_opt->label = _("Subtract raster values from the z coordinates");
    base_raster_opt->description = _("The scale for z is applied beforehand, the filter afterwards");
    base_raster_opt->guisection = _("Transform");

    zrange_opt = G_define_option();
    zrange_opt->key = "zrange";
    zrange_opt->type = TYPE_DOUBLE;
    zrange_opt->required = NO;
    zrange_opt->key_desc = "min,max";
    zrange_opt->description = _("Filter range for z data (min,max)");
    zrange_opt->guisection = _("Selection");

    zscale_opt = G_define_option();
    zscale_opt->key = "zscale";
    zscale_opt->type = TYPE_DOUBLE;
    zscale_opt->required = NO;
    zscale_opt->answer = "1.0";
    zscale_opt->description = _("Scale to apply to z data");
    zscale_opt->guisection = _("Transform");

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
    trim_opt->label = _("Discard given percentage of the smallest and largest values");
    trim_opt->description =
	_("Discard <trim> percent of the smallest and <trim> percent of the largest observations");
    trim_opt->guisection = _("Statistic");

    res_opt = G_define_option();
    res_opt->key = "resolution";
    res_opt->type = TYPE_DOUBLE;
    res_opt->required = NO;
    res_opt->description =
	_("Output raster resolution");

    filter_opt = G_define_option();
    filter_opt->key = "return_filter";
    filter_opt->type = TYPE_STRING;
    filter_opt->required = NO;
    filter_opt->label = _("Only import points of selected return type");
    filter_opt->description = _("If not specified, all points are imported");
    filter_opt->options = "first,last,mid";
    filter_opt->guisection = _("Selection");

    class_opt = G_define_option();
    class_opt->key = "class_filter";
    class_opt->type = TYPE_INTEGER;
    class_opt->multiple = YES;
    class_opt->required = NO;
    class_opt->label = _("Only import points of selected class(es)");
    class_opt->description = _("Input is comma separated integers. "
                               "If not specified, all points are imported.");
    class_opt->guisection = _("Selection");

    print_flag = G_define_flag();
    print_flag->key = 'p';
    print_flag->description =
	_("Print LAS file info and exit");
    print_flag->suppress_required = YES;

    extents_flag = G_define_flag();
    extents_flag->key = 'e';
    extents_flag->description =
	_("Extend region extents based on new dataset");

    over_flag = G_define_flag();
    over_flag->key = 'o';
    over_flag->label =
	_("Override projection check (use current location's projection)");
    over_flag->description =
	_("Assume that the dataset has same projection as the current location");

    scan_flag = G_define_flag();
    scan_flag->key = 's';
    scan_flag->description = _("Scan data file for extent then exit");
    scan_flag->suppress_required = YES;

    shell_style = G_define_flag();
    shell_style->key = 'g';
    shell_style->description =
	_("In scan mode, print using shell script style");

    intens_flag = G_define_flag();
    intens_flag->key = 'i';
    intens_flag->description =
        _("Import intensity values rather than z values");

    base_rast_res_flag = G_define_flag();
    base_rast_res_flag->key = 'd';
    base_rast_res_flag->description =
        _("Use base raster actual resolution instead of computational region");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* parse input values */
    infile = input_opt->answer;
    outmap = output_opt->answer;

    if (shell_style->answer && !scan_flag->answer) {
	scan_flag->answer = 1; /* pointer not int, so set = shell_style->answer ? */
    }

    /* Don't crash on cmd line if file not found */
    if (access(infile, F_OK) != 0) {
	G_fatal_error(_("Input file <%s> does not exist"), infile);
    }
    /* Open LAS file*/
    LAS_reader = LASReader_Create(infile);
    if (LAS_reader == NULL) {
	G_fatal_error(_("Unable to open file <%s>"), infile);
    }
    
    LAS_header = LASReader_GetHeader(LAS_reader);
    if  (LAS_header == NULL) {
	G_fatal_error(_("Input file <%s> is not a LAS LiDAR point cloud"),
	                infile);
    }

    LAS_srs = LASHeader_GetSRS(LAS_header);

    /* Print LAS header */
    if (print_flag->answer) {
	/* print... */
	print_lasinfo(LAS_header, LAS_srs);

	LASSRS_Destroy(LAS_srs);
	LASHeader_Destroy(LAS_header);
	LASReader_Destroy(LAS_reader);

	exit(EXIT_SUCCESS);
    }

    return_filter = LAS_ALL;
    if (filter_opt->answer) {
	if (strcmp(filter_opt->answer, "first") == 0)
	    return_filter = LAS_FIRST;
	else if (strcmp(filter_opt->answer, "last") == 0)
	    return_filter = LAS_LAST;
	else if (strcmp(filter_opt->answer, "mid") == 0)
	    return_filter = LAS_MID;
	else
	    G_fatal_error(_("Unknown filter option <%s>"), filter_opt->answer);
    }

    /* Fetch input map projection in GRASS form. */
    projstr = LASSRS_GetWKT_CompoundOK(LAS_srs);

    if (TRUE) {
        projection_check_wkt(cellhd, loc_wind, projstr, over_flag->answer, shell_style->answer);
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
    base_array = NULL;
    
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

    /* using row-based chunks (used for output) when input and output
     * region matches and using segment library when they don't */
    int use_segment = 0;
    int use_base_raster_res = 0;
    if (base_rast_res_flag->answer)
        use_base_raster_res = 1;
    if (base_raster_opt->answer && (res_opt->answer || use_base_raster_res))
        use_segment = 1;
    if (base_raster_opt->answer && !use_segment) {
        /* TODO: do we need to test existence first? mapset? */
        base_raster = Rast_open_old(base_raster_opt->answer, "");
        base_raster_data_type = Rast_get_map_type(base_raster);
        base_array = G_calloc((size_t)rows * (cols + 1), Rast_cell_size(base_raster_data_type));
    }
    if (base_raster_opt->answer && use_segment) {
        if (use_base_raster_res) {
            /* TODO: how to get cellhd already stored in the open map? */
            Rast_get_cellhd(base_raster_opt->answer, "", &input_region);
            /* TODO: make it only as small as the output is or points are */
            Rast_set_input_window(&input_region);  /* we have split window */
        }
        rast_segment_open(&base_segment, base_raster_opt->answer, &base_raster_data_type);
    }

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
	/* TODO: perhaps none of them needs to be freed */

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

        if (base_array) {
            G_debug(2, "filling base raster array");
            for (row = 0; row < rows; row++) {
                Rast_get_row(base_raster, base_array + (row * cols * Rast_cell_size(base_raster_data_type)), row, base_raster_data_type);
            }
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

	    if (counter == 100000) {	/* speed */
		if (line < estimated_lines)
		    G_percent(line, estimated_lines, 3);
		counter = 0;
	    }

	    if (!LASPoint_IsValid(LAS_point)) {
		continue;
	    }

	    x = LASPoint_GetX(LAS_point);
	    y = LASPoint_GetY(LAS_point);
	    if (intens_flag->answer)
		/* use z variable here to allow for scaling of intensity below */
		z = LASPoint_GetIntensity(LAS_point);
	    else
		z = LASPoint_GetZ(LAS_point);

	if (return_filter != LAS_ALL) {
	    int return_no = LASPoint_GetReturnNumber(LAS_point);
	    int n_returns = LASPoint_GetNumberOfReturns(LAS_point);
	    skipme = 1;

	    switch (return_filter) {
	    case LAS_FIRST:
		if (return_no == 1)
		    skipme = 0;
		break;
	    case LAS_MID:
		if (return_no > 1 && return_no < n_returns)
		    skipme = 0;
		break;
	    case LAS_LAST:
		if (n_returns > 1 && return_no == n_returns)
		    skipme = 0;
		break;
	    }

	    if (skipme) {
		n_filtered++;
		continue;
	    }
	}
	if (class_opt->answer) {
	    point_class = (int) LASPoint_GetClassification(LAS_point);
	    i = 0;
	    skipme = TRUE;
	    while (class_opt->answers[i]) {
		if (point_class == atoi(class_opt->answers[i])) {
		    skipme = FALSE;
		    break;
		}
		i++;
	    }
	    if (skipme) {
		continue;
	    }
	}

	    if (y <= pass_south || y > pass_north) {
		continue;
	    }
	    if (x < region.west || x >= region.east) {
		continue;
	    }

	    z = z * zscale;

            /* find the bin in the current array box */
            arr_row = (int)((pass_north - y) / region.ns_res);
            arr_col = (int)((x - region.west) / region.ew_res);

            if (base_array) {
                double base_z;
                if (row_array_get_value_row_col(base_array, arr_row, arr_col,
                                                cols, base_raster_data_type,
                                                &base_z))
                    z -= base_z;
                else
                    continue;
            }
            else if (use_segment) {
                double base_z;
                if (rast_segment_get_value_xy(&base_segment, &input_region,
                                              base_raster_data_type, x, y,
                                              &base_z))
                    z -= base_z;
                else
                    continue;
            }

            if (zrange_opt->answer) {
                if (z < zrange_min || z > zrange_max) {
                    continue;
                }
            }

	    count++;
	    /*          G_debug(5, "x: %f, y: %f, z: %f", x, y, z); */

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
    if (base_array)
        Rast_close(base_raster);
    if (use_segment)
        Segment_close(&base_segment);

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
    G_done_msg("%s", buff);
    G_debug(1, "Processed %lu points.", line_total);

    exit(EXIT_SUCCESS);

}
