
/****************************************************************************
 *
 * MODULE:       r.stream.extract
 * AUTHOR(S):    Markus Metz <markus.metz.giswork gmail.com>
 * PURPOSE:      Hydrological analysis
 *               Extracts stream networks from accumulation raster with
 *               given threshold
 * COPYRIGHT:    (C) 1999-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

/* global variables */
int nrows, ncols;
GW_LARGE_INT n_search_points, n_points, nxt_avail_pt;
GW_LARGE_INT heap_size;
GW_LARGE_INT n_stream_nodes, n_alloc_nodes;
POINT *outlets;
struct snode *stream_node;
GW_LARGE_INT n_outlets, n_alloc_outlets;
char drain[3][3] = { {7, 6, 5}, {8, 0, 4}, {1, 2, 3} };
char sides;
int c_fac;
int ele_scale;
int have_depressions;

SSEG search_heap;
SSEG astar_pts;
SSEG watalt, aspflag;
/* BSEG bitflags, asp; */
CSEG stream;

CELL *astar_order;

int main(int argc, char *argv[])
{
    struct
    {
	struct Option *ele, *acc, *depression;
	struct Option *threshold, *d8cut;
	struct Option *mont_exp;
	struct Option *min_stream_length;
	struct Option *memory;
    } input;
    struct
    {
	struct Option *stream_rast;
	struct Option *stream_vect;
	struct Option *dir_rast;
    } output;
    struct GModule *module;
    int ele_fd, acc_fd, depr_fd;
    double threshold, d8cut, mont_exp;
    int min_stream_length = 0, memory;
    int seg_cols, seg_rows;
    double seg2kb;
    int num_open_segs, num_open_array_segs, num_seg_total;
    double memory_divisor, heap_mem, disk_space;
    const char *mapset;

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("stream network"));
    module->description = _("Performs stream network extraction.");

    input.ele = G_define_standard_option(G_OPT_R_ELEV);

    input.acc = G_define_standard_option(G_OPT_R_INPUT);
    input.acc->key = "accumulation";
    input.acc->label = _("Name of input accumulation raster map");
    input.acc->required = NO;
    input.acc->description =
	_("Stream extraction will use provided accumulation instead of calculating it a new");
    input.acc->guisection = _("Input options");

    input.depression = G_define_standard_option(G_OPT_R_INPUT);
    input.depression->key = "depression";
    input.depression->label = _("Name of raster map with real depressions");
    input.depression->required = NO;
    input.depression->description =
	_("Streams will not be routed out of real depressions");
    input.depression->guisection = _("Input options");

    input.threshold = G_define_option();
    input.threshold->key = "threshold";
    input.threshold->label = _("Minimum flow accumulation for streams");
    input.threshold->description = _("Must be > 0");
    input.threshold->required = YES;
    input.threshold->type = TYPE_DOUBLE;

    input.d8cut = G_define_option();
    input.d8cut->key = "d8cut";
    input.d8cut->label = _("Use SFD above this threshold");
    input.d8cut->description =
	_("If accumulation is larger than d8cut, SFD is used instead of MFD."
	  " Applies only if no accumulation map is given.");
    input.d8cut->required = NO;
    input.d8cut->answer = "infinity";
    input.d8cut->type = TYPE_DOUBLE;

    input.mont_exp = G_define_option();
    input.mont_exp->key = "mexp";
    input.mont_exp->type = TYPE_DOUBLE;
    input.mont_exp->required = NO;
    input.mont_exp->answer = "0";
    input.mont_exp->label =
	_("Montgomery exponent for slope, disabled with 0");
    input.mont_exp->description =
	_("Montgomery: accumulation is multiplied with pow(slope,mexp) and then compared with threshold.");

    input.min_stream_length = G_define_option();
    input.min_stream_length->key = "stream_length";
    input.min_stream_length->type = TYPE_INTEGER;
    input.min_stream_length->required = NO;
    input.min_stream_length->answer = "0";
    input.min_stream_length->label =
	_("Delete stream segments shorter than stream_length cells.");
    input.min_stream_length->description =
	_("Applies only to first-order stream segments (springs/stream heads).");

    input.memory = G_define_option();
    input.memory->key = "memory";
    input.memory->type = TYPE_INTEGER;
    input.memory->required = NO;
    input.memory->answer = "300";
    input.memory->description = _("Maximum memory to be used in MB");

    output.stream_rast = G_define_standard_option(G_OPT_R_OUTPUT);
    output.stream_rast->key = "stream_rast";
    output.stream_rast->description =
	_("Name for output raster map with unique stream ids");
    output.stream_rast->required = NO;
    output.stream_rast->guisection = _("Output options");

    output.stream_vect = G_define_standard_option(G_OPT_V_OUTPUT);
    output.stream_vect->key = "stream_vect";
    output.stream_vect->description =
	_("Name for output vector map with unique stream ids");
    output.stream_vect->required = NO;
    output.stream_vect->guisection = _("Output options");

    output.dir_rast = G_define_standard_option(G_OPT_R_OUTPUT);
    output.dir_rast->key = "direction";
    output.dir_rast->description =
	_("Name for output raster map with flow direction");
    output.dir_rast->required = NO;
    output.dir_rast->guisection = _("Output options");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /***********************/
    /*    check options    */
    /***********************/

    /* input maps exist ? */
    if (!G_find_raster(input.ele->answer, ""))
	G_fatal_error(_("Raster map <%s> not found"), input.ele->answer);

    if (input.acc->answer) {
	if (!G_find_raster(input.acc->answer, ""))
	    G_fatal_error(_("Raster map <%s> not found"), input.acc->answer);
    }

    if (input.depression->answer) {
	if (!G_find_raster(input.depression->answer, ""))
	    G_fatal_error(_("Raster map <%s> not found"), input.depression->answer);
	have_depressions = 1;
    }
    else
	have_depressions = 0;

    /* threshold makes sense */
    threshold = atof(input.threshold->answer);
    if (threshold <= 0)
	G_fatal_error(_("Threshold must be > 0 but is %f"), threshold);

    /* d8cut */
    if (strcmp(input.d8cut->answer, "infinity") == 0) {
	d8cut = DBL_MAX;
    }
    else {
	d8cut = atof(input.d8cut->answer);
	if (d8cut < 0)
	    G_fatal_error(_("d8cut must be positive or zero but is %f"),
			  d8cut);
    }

    /* Montgomery stream initiation */
    if (input.mont_exp->answer) {
	mont_exp = atof(input.mont_exp->answer);
	if (mont_exp < 0)
	    G_fatal_error(_("Montgomery exponent must be positive or zero but is %f"),
			  mont_exp);
	if (mont_exp > 3)
	    G_warning(_("Montgomery exponent is %f, recommended range is 0.0 - 3.0"),
		      mont_exp);
    }
    else
	mont_exp = 0;

    /* Minimum stream segment length */
    if (input.min_stream_length->answer) {
	min_stream_length = atoi(input.min_stream_length->answer);
	if (min_stream_length < 0)
	    G_fatal_error(_("Minimum stream length must be positive or zero but is %d"),
			  min_stream_length);
    }
    else
	min_stream_length = 0;
	
    if (input.memory->answer) {
	memory = atoi(input.memory->answer);
	if (memory <= 0)
	    G_fatal_error(_("Memory must be positive but is %d"),
			  memory);
    }
    else
	memory = 300;

    /* Check for some output map */
    if ((output.stream_rast->answer == NULL)
	&& (output.stream_vect->answer == NULL)
	&& (output.dir_rast->answer == NULL)) {
	G_fatal_error(_("Sorry, you must choose at least one output map."));
    }

    /*********************/
    /*    preparation    */
    /*********************/

    /* open input maps */
    mapset = G_find_raster2(input.ele->answer, "");
    ele_fd = Rast_open_old(input.ele->answer, mapset);
    if (ele_fd < 0)
	G_fatal_error(_("Could not open input map %s"), input.ele->answer);

    if (input.acc->answer) {
	mapset = G_find_raster2(input.acc->answer, "");
	acc_fd = Rast_open_old(input.acc->answer, mapset);
	if (acc_fd < 0)
	    G_fatal_error(_("Could not open input map %s"),
			  input.acc->answer);
    }
    else
	acc_fd = -1;

    if (input.depression->answer) {
	mapset = G_find_raster2(input.depression->answer, "");
	depr_fd = Rast_open_old(input.depression->answer, mapset);
	if (depr_fd < 0)
	    G_fatal_error(_("Could not open input map %s"),
			  input.depression->answer);
    }
    else
	depr_fd = -1;

    /* set global variables */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    sides = 8;	/* not a user option */
    c_fac = 5;	/* not a user option, MFD covergence factor 5 gives best results */

    /* segment structures */
    seg_rows = seg_cols = 64;
    seg2kb = seg_rows * seg_cols / 1024.;

    /* balance segment files */
    /* elevation + accumulation: * 2 */
    memory_divisor = sizeof(WAT_ALT) * 2;
    disk_space = sizeof(WAT_ALT);
    /* stream ids: / 2 */
    memory_divisor += sizeof(int) / 2.;
    disk_space += sizeof(int);

    /* aspect and flags: * 2 */
    memory_divisor += sizeof(ASP_FLAG) * 4;
    disk_space += sizeof(ASP_FLAG);

    /* astar_points: / 16 */
    /* ideally only a few but large segments */
    memory_divisor += sizeof(POINT) / 16.;
    disk_space += sizeof(POINT);
    /* heap points: / 4 */
    memory_divisor += sizeof(HEAP_PNT) / 4.;
    disk_space += sizeof(HEAP_PNT);
    
    /* KB -> MB */
    memory_divisor *= seg2kb / 1024.;
    disk_space *= seg2kb / 1024.;

    num_open_segs = memory / memory_divisor;
    heap_mem = num_open_segs * seg2kb * sizeof(HEAP_PNT) /
               (4. * 1024.);
    num_seg_total = (ncols / seg_cols + 1) * (nrows / seg_rows + 1);
    if (num_open_segs > num_seg_total) {
	heap_mem += (num_open_segs - num_seg_total) * memory_divisor;
	heap_mem -= (num_open_segs - num_seg_total) * seg2kb *
		    sizeof(HEAP_PNT) / (4. * 1024.);
	num_open_segs = num_seg_total;
    }
    if (num_open_segs < 16) {
	num_open_segs = 16;
	heap_mem = num_open_segs * seg2kb * sizeof(HEAP_PNT) /
	           (4. * 1024.);
    }
    G_verbose_message(_("%.2f%% of data are kept in memory"),
                      100. * num_open_segs / num_seg_total);
    disk_space *= num_seg_total;
    if (disk_space < 1024.0)
	G_verbose_message(_("Will need up to %.2f MB of disk space"), disk_space);
    else
	G_verbose_message(_("Will need up to %.2f GB (%.0f MB) of disk space"),
	           disk_space / 1024.0, disk_space);

    /* open segment files */
    G_verbose_message(_("Creating temporary files..."));
    seg_open(&watalt, nrows, ncols, seg_rows, seg_cols, num_open_segs * 2,
        sizeof(WAT_ALT), 1);
    if (num_open_segs * 2 > num_seg_total)
	heap_mem += (num_open_segs * 2 - num_seg_total) * seg2kb *
	            sizeof(WAT_ALT) / 1024.;
    cseg_open(&stream, seg_rows, seg_cols, num_open_segs / 2.);

    seg_open(&aspflag, nrows, ncols, seg_rows, seg_cols, num_open_segs * 2,
        sizeof(ASP_FLAG), 1);
/*
    bseg_open(&asp, seg_rows, seg_cols, num_open_segs);
    bseg_open(&bitflags, seg_rows, seg_cols, num_open_segs * 4);
*/

    if (num_open_segs * 4 > num_seg_total)
	heap_mem += (num_open_segs * 4 - num_seg_total) * seg2kb / 1024.;

    /* load maps */
    if (load_maps(ele_fd, acc_fd) < 0)
	G_fatal_error(_("Could not load input map(s)"));
    else if (!n_points)
	G_fatal_error(_("No non-NULL cells in input map(s)"));

    G_debug(1, "open segments for A* points");
    /* columns per segment */
    seg_cols = seg_rows * seg_rows;
    num_seg_total = n_points / seg_cols;
    if (n_points % seg_cols > 0)
	num_seg_total++;
    /* no need to have more segments open than exist */
    num_open_array_segs = num_open_segs / 16.;
    if (num_open_array_segs > num_seg_total)
	num_open_array_segs = num_seg_total;
    if (num_open_array_segs < 1)
	num_open_array_segs = 1;
    
    G_debug(1, "segment size for A* points: %d", seg_cols);
    seg_open(&astar_pts, 1, n_points, 1, seg_cols, num_open_array_segs,
	     sizeof(POINT), 1);

    /* one-based d-ary search_heap with astar_pts */
    G_debug(1, "open segments for A* search heap");
	
    /* allowed memory for search heap in MB */
    G_debug(1, "heap memory %.2f MB", heap_mem);
    /* columns per segment */
    /* larger is faster */
    seg_cols = seg_rows * seg_rows * seg_rows;
    num_seg_total = n_points / seg_cols;
    if (n_points % seg_cols > 0)
	num_seg_total++;
    /* no need to have more segments open than exist */
    num_open_array_segs = (1 << 20) * heap_mem / (seg_cols * sizeof(HEAP_PNT));
    if (num_open_array_segs > num_seg_total)
	num_open_array_segs = num_seg_total;
    if (num_open_array_segs < 2)
	num_open_array_segs = 2;

    G_debug(1, "A* search heap open segments %d, total %d",
            num_open_array_segs, num_seg_total);
    G_debug(1, "segment size for heap points: %d", seg_cols);
    /* the search heap will not hold more than 5% of all points at any given time ? */
    /* chances are good that the heap will fit into one large segment */
    seg_open(&search_heap, 1, n_points + 1, 1, seg_cols,
	     num_open_array_segs, sizeof(HEAP_PNT), 1);

    /********************/
    /*    processing    */
    /********************/

    /* initialize A* search */
    if (init_search(depr_fd) < 0)
	G_fatal_error(_("Could not initialize search"));

    /* sort elevation and get initial stream direction */
    if (do_astar() < 0)
	G_fatal_error(_("Could not sort elevation map"));
    seg_close(&search_heap);

    if (acc_fd < 0) {
	/* accumulate surface flow */
	if (do_accum(d8cut) < 0)
	    G_fatal_error(_("Could not calculate flow accumulation"));
    }

    /* extract streams */
    if (extract_streams(threshold, mont_exp, acc_fd < 0) < 0)
	G_fatal_error(_("Could not extract streams"));

    seg_close(&astar_pts);
    seg_close(&watalt);

    /* thin streams */
    if (thin_streams() < 0)
	G_fatal_error(_("Could not thin streams"));

    /* delete short streams */
    if (min_stream_length) {
	if (del_streams(min_stream_length) < 0)
	    G_fatal_error(_("Could not delete short stream segments"));
    }

    /* write output maps */
    if (close_maps(output.stream_rast->answer, output.stream_vect->answer,
		   output.dir_rast->answer) < 0)
	G_fatal_error(_("Could not write output maps"));

    cseg_close(&stream);
    seg_close(&aspflag);

    exit(EXIT_SUCCESS);
}
