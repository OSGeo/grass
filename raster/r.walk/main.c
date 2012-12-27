
/****************************************************************************
 *
 * MODULE:       r.walk
 * AUTHOR(S):    Based on r.cost written by :
 *                 Antony Awaida,
 *                 Intelligent Engineering
 *                 Systems Laboratory,
 *                 M.I.T.
 *                 James Westervelt,
 *                 U.S.Army Construction Engineering Research Laboratory
 *
 *               Updated for Grass 5
 *                 Pierre de Mouveaux (pmx@audiovu.com)
 *
 *               Initial version of r.walk:
 *                 Steno Fontanari, 2002, ITC-irst
 *
 *               GRASS 6.0 version of r.walk:
 *                 Franceschetti Simone, Sorrentino Diego, Mussi Fabiano and Pasolli Mattia
 *                 Correction by: Fontanari Steno, Napolitano Maurizio and  Flor Roberto
 *                 In collaboration with: Franchi Matteo, Vaglia Beatrice, Bartucca Luisa,
 *                    Fava  Valentina  and Tolotti Mathias, 2004
 *
 *               Updated for GRASS 6.1
 *                 Roberto Flor and Markus Neteler
 *                 Glynn Clements <glynn gclements.plus.com>, Soeren Gebbert <soeren.gebbert gmx.de>
 *               Updated for calculation errors and directional surface generation
 *                 Colin Nielsen <colin.nielsen gmail com>
 *               Updated for GRASS 7
 *                 Markus Metz
 * PURPOSE:      anisotropic movements on cost surfaces
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/*********************************************************************
 *
 *     This is the main program for the minimum path cost analysis.
 *     It generates a cumulative cost map (output) from an elevation (inputdtm
 *     and cost map (inputcost) with respect to starting locations (coor).
 *
 *     It takes as input the following:
 *     1) Cost of traversing each grid cell as given by an elevation map and
 *        a cost map cell (inputcost).
 *     2) If starting points are not specified on the command line
 *        then the output map must exist and contain the starting locations
 *     
 *        Otherwise the output map need not exist and the coor points
 *        from the command line are used.
 *
 *********************************************************************/

/*********************************************************************
 *   The walking energy is computed for the human walk, based on Aitken,
 * 1977, Langmuir, 1984:
 *
 *                {T= [(a)x(Delta S)] + [(b)x(Delta H Climb)]
 *			+[(c)*(Delta H moderate downhill)]+[(d)*(Delta H steep downhill]}
 *
 *		where T is time in seconds, Delta S distance in meter, Delta H the heigth difference
 *
 *  The default	a,b,c,d parameters used below have been measured using oxygen consumption in biomechanical 
 *  experiments.
 *  Refs:
 *       * Aitken, R. 1977. Wilderness areas in Scotland. Unpublished Ph.D. thesis. University of Aberdeen.
 *       * Steno Fontanari, University of Trento, Italy, Ingegneria per l'Ambiente e
 *         il Territorio, 2000-2001. Svilluppo di metodologie GIS per la determinazione dell'accessibilita'
 *         territoriale come supporto alle decisioni nella gestione ambientale.
 *       * Langmuir, E. 1984. Mountaincraft and leadership. The Scottish Sports Council/MLTB. Cordee,
 *         Leicester.
 *
 *   The total cost is computed as a linear combination of walking energy and a given friction cost map:
 *
 *	 TOTAL COST = [(WALKING ENERGY ) + (LAMBDA*FRICTION)]
 *
 * TODO: generalize formula to other species
 *************/
/*  
 * 
 * 20 july 2004 - Pierre de Mouveaux. pmx@audiovu.com
 * Updated to use the Grass 5.0 floating point raster cell format.
 * Convert floats to double. Done ;)
 * 2001: original r.walk by Steno Fontanari, ITC-irst
 * 24 July 2004: WebValley 2004, fixed and enhanced by
 * Matteo Franchi               Liceo Leonardo Da Vinci Trento
 * Roberto Flor         ITC-irst
 * 7 December 2005: Grass 6.1 cleanup
 * Roberto Flor         ITC-irst
 * Markus Neteler               CEA
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/segment.h>
#include <grass/glocale.h>
#include "cost.h"
#include "stash.h"

#define SEGCOLSIZE 	64

struct Cell_head window;

struct start_pt *head_start_pt = NULL;
struct start_pt *head_end_pt = NULL;

int main(int argc, char *argv[])
{
    const char *cum_cost_layer, *move_dir_layer;
    const char *cost_layer, *dtm_layer;
    const char *dtm_mapset, *cost_mapset, *search_mapset;
    void *dtm_cell, *cost_cell, *cum_cell, *dir_cell, *cell2 = NULL;
    SEGMENT cost_seg, dir_seg;
    const char *in_file, *dir_out_file = NULL;
    double *value;
    char buf[400];
    extern struct Cell_head window;
    double NS_fac, EW_fac, DIAG_fac, H_DIAG_fac, V_DIAG_fac;
    double fcost_dtm, fcost_cost;
    double min_cost, old_min_cost;
    FCELL cur_dir;
    double zero = 0.0;
    int col = 0, row = 0, nrows = 0, ncols = 0;
    int maxcost, par_number;
    int nseg;
    int maxmem;
    int segments_in_memory;
    int cost_fd, cum_fd, dtm_fd, dir_fd;
    int have_stop_points = 0, dir = 0;
    int in_fd, dir_out_fd = 0;
    double my_dtm, my_cost, check_dtm;
    double null_cost, dnullval;
    double a, b, c, d, lambda, slope_factor;
    int srows, scols;
    int total_reviewed;
    int keep_nulls = 1;
    int start_with_raster_vals = 1;
    int neighbor;
    long n_processed = 0;
    long total_cells;
    struct GModule *module;
    struct Flag *flag2, *flag3, *flag4, *flag5;
    struct Option *opt1, *opt2, *opt3, *opt4, *opt5, *opt6, *opt7, *opt8;
    struct Option *opt9, *opt10, *opt11, *opt12, *opt13, *opt14, *opt15;
    struct cost *pres_cell, *new_cell;
    struct start_pt *pres_start_pt = NULL;
    struct start_pt *pres_stop_pt = NULL;
    struct cc {
	double dtm;		/* elevation model */
	double cost_in;		/* friction costs */
	double cost_out;	/* cumulative costs */
    } costs;

    void *ptr1, *ptr2;
    RASTER_MAP_TYPE dtm_data_type, cost_data_type, cum_data_type =
	DCELL_TYPE, dir_data_type = FCELL_TYPE;
    struct History history;
    double peak = 0.0;
    int dtm_dsize, cost_dsize;

    /* Definition for dimension and region check */
    struct Cell_head dtm_cellhd, cost_cellhd;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("cost surface"));
    G_add_keyword(_("cumulative costs"));
    module->description =
	_("Outputs a raster map layer showing the "
	  "anisotropic cumulative cost of moving between different "
	  "geographic locations on an input elevation raster map "
	  "layer whose cell category values represent elevation "
	  "combined with an input raster map layer whose cell "
	  "values represent friction cost.");

    opt12 = G_define_standard_option(G_OPT_R_INPUT);
    opt12->key = "elevation";
    opt12->required = YES;
    opt12->description = _("Name of elevation input raster map");

    opt2 = G_define_standard_option(G_OPT_R_INPUT);
    opt2->key = "friction";
    opt2->required = YES;
    opt2->description =
	_("Name of input raster map containing friction costs");

    opt1 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt1->required = YES;
    opt1->label = _("output map with walking costs");
    opt1->description = _("Name of output raster map to contain walking costs");

    opt11 = G_define_option();
    opt11->key = "outdir";
    opt11->type = TYPE_STRING;
    opt11->required = NO;
    opt11->gisprompt = "new,cell,raster";
    opt11->description =
	_("Name of output raster map to contain movement directions");

    opt7 = G_define_option();
    opt7->key = "start_points";
    opt7->type = TYPE_STRING;
    opt7->gisprompt = "old,vector,vector";
    opt7->required = NO;
    opt7->description = _("Starting points vector map");

    opt8 = G_define_option();
    opt8->key = "stop_points";
    opt8->type = TYPE_STRING;
    opt8->gisprompt = "old,vector,vector";
    opt8->required = NO;
    opt8->description = _("Stop points vector map");

    opt9 = G_define_option();
    opt9->key = "start_rast";
    opt9->type = TYPE_STRING;
    opt9->required = NO;
    opt9->gisprompt = "old,cell,raster";
    opt9->description =
	_("Starting points raster map");

    opt3 = G_define_option();
    opt3->key = "coordinate";
    opt3->type = TYPE_STRING;
    opt3->key_desc = "x,y";
    opt3->multiple = YES;
    opt3->description =
	_("The map E and N grid coordinates of a starting point (E,N)");

    opt4 = G_define_option();
    opt4->key = "stop_coordinate";
    opt4->type = TYPE_STRING;
    opt4->key_desc = "x,y";
    opt4->multiple = YES;
    opt4->description =
	_("The map E and N grid coordinates of a stopping point (E,N)");

    opt5 = G_define_option();
    opt5->key = "max_cost";
    opt5->type = TYPE_INTEGER;
    opt5->required = NO;
    opt5->multiple = NO;
    opt5->answer = "0";
    opt5->description = _("An optional maximum cumulative cost");

    opt6 = G_define_option();
    opt6->key = "null_cost";
    opt6->type = TYPE_DOUBLE;
    opt6->required = NO;
    opt6->multiple = NO;
    opt6->description =
	_("Cost assigned to null cells. By default, null cells are excluded");

    opt10 = G_define_option();
    opt10->key = "percent_memory";
    opt10->type = TYPE_INTEGER;
    opt10->required = NO;
    opt10->multiple = NO;
    opt10->answer = "100";
    opt10->description = _("Percent of map to keep in memory");

    opt15 = G_define_option();
    opt15->key = "walk_coeff";
    opt15->type = TYPE_STRING;
    opt15->key_desc = "a,b,c,d";
    opt15->required = NO;
    opt15->multiple = NO;
    opt15->answer = "0.72,6.0,1.9998,-1.9998";
    opt15->description =
	_("Coefficients for walking energy formula parameters a,b,c,d");

    opt14 = G_define_option();
    opt14->key = "lambda";
    opt14->type = TYPE_DOUBLE;
    opt14->required = NO;
    opt14->multiple = NO;
    opt14->answer = "1.0";
    opt14->description =
	_("Lambda coefficients for combining walking energy and friction cost");

    opt13 = G_define_option();
    opt13->key = "slope_factor";
    opt13->type = TYPE_DOUBLE;
    opt13->required = NO;
    opt13->multiple = NO;
    opt13->answer = "-0.2125";
    opt13->description =
	_("Slope factor determines travel energy cost per height step");

    flag2 = G_define_flag();
    flag2->key = 'k';
    flag2->description =
	_("Use the 'Knight's move'; slower, but more accurate");

    flag3 = G_define_flag();
    flag3->key = 'n';
    flag3->description = _("Keep null values in output map");

    flag4 = G_define_flag();
    flag4->key = 'r';
    flag4->description = _("Start with values in raster map");

    flag5 = G_define_flag();
    flag5->key = 'i';
    flag5->description = _("Only print info about disk space and memory requirements");

    /* Parse options */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* If no outdir is specified, set flag to skip all dir */
    if (opt11->answer != NULL)
	dir = 1;

    /* Initalize access to database and create temporary files */
    in_file = G_tempfile();
    if (dir == 1)
	dir_out_file = G_tempfile();

    /* Get database window parameters */
    Rast_get_window(&window);

    /* Find north-south, east_west and diagonal factors */
    EW_fac = window.ew_res;	/* Must be the physical distance */
    NS_fac = window.ns_res;
    DIAG_fac = (double)sqrt((double)(NS_fac * NS_fac + EW_fac * EW_fac));
    V_DIAG_fac =
	(double)sqrt((double)(4 * NS_fac * NS_fac + EW_fac * EW_fac));
    H_DIAG_fac =
	(double)sqrt((double)(NS_fac * NS_fac + 4 * EW_fac * EW_fac));

    Rast_set_d_null_value(&null_cost, 1);

    if (flag2->answer)
	total_reviewed = 16;
    else
	total_reviewed = 8;

    keep_nulls = flag3->answer;

    start_with_raster_vals = flag4->answer;

    {
	int count = 0;

	if (opt3->answers)
	    count++;
	if (opt7->answers)
	    count++;
	if (opt9->answers)
	    count++;

	if (count != 1)
	    G_fatal_error(_("Must specify exactly one of start_points, start_rast or coordinate"));
    }

    if (opt3->answers)
	if (!process_answers(opt3->answers, &head_start_pt, &pres_start_pt))
	    G_fatal_error(_("No start points"));

    if (opt4->answers)
	have_stop_points =
	    process_answers(opt4->answers, &head_end_pt, &pres_stop_pt);

    if (sscanf(opt5->answer, "%d", &maxcost) != 1 || maxcost < 0)
	G_fatal_error(_("Inappropriate maximum cost: %d"), maxcost);

    if (sscanf(opt10->answer, "%d", &maxmem) != 1 || maxmem < 0 ||
	maxmem > 100)
	G_fatal_error(_("Inappropriate percent memory: %d"), maxmem);

    /* Getting walking energy formula parameters */
    if ((par_number =
	 sscanf(opt15->answer, "%lf,%lf,%lf,%lf", &a, &b, &c, &d)) != 4)
	G_fatal_error(_("Missing required value: got %d instead of 4"),
		      par_number);
    else {
	G_message(_("Walking costs are a=%lf b=%lf c=%lf d=%lf"), a, b, c, d);
    }

    /* Getting lambda */
    if ((par_number = sscanf(opt14->answer, "%lf", &lambda)) != 1)
	G_fatal_error(_("Missing required value: %d"), par_number);
    else {

	G_message(_("Lambda is %lf"), lambda);
    }

    /* Getting slope_factor */
    if ((par_number = sscanf(opt13->answer, "%lf", &slope_factor)) != 1)
	G_fatal_error(_("Missing required value: %d"), par_number);
    else {

	G_message(_("Slope_factor is %lf"), slope_factor);
    }

    if ((opt6->answer == NULL) ||
	(sscanf(opt6->answer, "%lf", &null_cost) != 1)) {
	G_debug(1, "Null cells excluded from cost evaluation");
	Rast_set_d_null_value(&null_cost, 1);
    }
    else if (keep_nulls)
	G_debug(1,"Input null cell will be retained into output map");

    if (opt7->answer) {
	search_mapset = G_find_vector2(opt7->answer, "");
	if (search_mapset == NULL)
	    G_fatal_error(_("Vector map <%s> not found"), opt7->answer);
    }

    if (!Rast_is_d_null_value(&null_cost)) {
	if (null_cost < 0.0) {
	    G_warning(_("Warning: assigning negative cost to null cell. Null cells excluded."));
	    Rast_set_d_null_value(&null_cost, 1);
	}
    }
    else {
	keep_nulls = 0;		/* handled automagically... */
    }

    dtm_layer = opt12->answer;
    cost_layer = opt2->answer;
    cum_cost_layer = opt1->answer;
    move_dir_layer = opt11->answer;

    /* Find number of rows and columns in window */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* Open cost cell layer for reading */
    dtm_mapset = G_find_raster2(dtm_layer, "");
    if (dtm_mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), dtm_layer);
    dtm_fd = Rast_open_old(dtm_layer, "");

    cost_mapset = G_find_raster2(cost_layer, "");
    if (cost_mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), cost_layer);
    cost_fd = Rast_open_old(cost_layer, cost_mapset);

    Rast_get_cellhd(dtm_layer, "", &dtm_cellhd);
    Rast_get_cellhd(cost_layer, "", &cost_cellhd);

    dtm_data_type = Rast_get_map_type(dtm_fd);
    cost_data_type = Rast_get_map_type(cost_fd);

    /* Parameters for map submatrices */
    switch (dtm_data_type) {
    case (CELL_TYPE):
	G_debug(1, "DTM_Source map is: Integer cell type");
	break;
    case (FCELL_TYPE):
	G_debug(1, "DTM_Source map is: Floating point (float) cell type");
	break;
    case (DCELL_TYPE):
	G_debug(1, "DTM_Source map is: Floating point (double) cell type");
	break;
    }
    G_debug(1, "DTM %d rows, %d cols", dtm_cellhd.rows, dtm_cellhd.cols);

    switch (cost_data_type) {
    case (CELL_TYPE):
	G_debug(1, "COST_Source map is: Integer cell type");
	break;
    case (FCELL_TYPE):
	G_debug(1, "COST_Source map is: Floating point (float) cell type");
	break;
    case (DCELL_TYPE):
	G_debug(1, "COST_Source map is: Floating point (double) cell type");
	break;
    }
    G_debug(1, "COST %d rows, %d cols", cost_cellhd.rows, cost_cellhd.cols);

    if (cost_data_type != dtm_data_type) {
	switch (cost_data_type) {
	case (CELL_TYPE):
	    if (dtm_data_type == FCELL_TYPE)
		cum_data_type = FCELL_TYPE;
	    else
		cum_data_type = DCELL_TYPE;
	    break;
	case (FCELL_TYPE):
	    if (dtm_data_type == DCELL_TYPE)
		cum_data_type = DCELL_TYPE;
	    else
		cum_data_type = FCELL_TYPE;
	    break;
	case (DCELL_TYPE):
	    cum_data_type = DCELL_TYPE;
	    break;
	}
    }
    else
	/* Data type are equal, it doesn't matter */
	cum_data_type = dtm_data_type;

    switch (cum_data_type) {
    case (CELL_TYPE):
	G_debug(1, "Output map is: Integer cell type");
	break;
    case (FCELL_TYPE):
	G_debug(1, "Output map is: Floating point (float) cell type");
	break;
    case (DCELL_TYPE):
	G_debug(1, "Output map is: Floating point (double) cell type");
	break;
    }
    G_debug(1, " %d rows, %d cols", nrows, ncols);
    G_format_resolution(window.ew_res, buf, window.proj);
    G_debug(1, " EW resolution %s (%lf)", buf, window.ew_res);
    G_format_resolution(window.ns_res, buf, window.proj);
    G_debug(1, " NS resolution %s (%lf)", buf, window.ns_res);

    /* this is most probably the limitation of r.walk for large datasets
     * segment size needs to be reduced to avoid unecessary disk IO
     * but it doesn't make sense to go down to 1
     * so use 64 segment rows and cols for <= 200 million cells
     * for larger regions, 32 segment rows and cols
     * maybe go down to 16 for > 500 million cells ? */
    if ((double) nrows * ncols > 200000000)
	srows = scols = SEGCOLSIZE / 2;
    else
	srows = scols = SEGCOLSIZE;

    if (maxmem == 100) {
	srows = scols = 256;
    }

    /* calculate total number of segments */
    nseg = ((nrows + srows - 1) / srows) * ((ncols + scols - 1) / scols);
    if (maxmem > 0)
	segments_in_memory = (maxmem * nseg) / 100;
    /* maxmem = 0 */
    else
	segments_in_memory = 4 * (nrows / srows + ncols / scols + 2);

    if (segments_in_memory == 0)
	segments_in_memory = 1;

    /* report disk space and memory requirements */
    G_message("--------------------------------------------");
    if (dir == 1) {
	double disk_mb, mem_mb;

	disk_mb = (double) nrows * ncols * 28. / 1048576.;
	mem_mb  = (double) srows * scols * 28. / 1048576. * segments_in_memory;
	mem_mb += nrows * ncols * 0.05 * 20. / 1048576.;    /* for Dijkstra search */
	G_message(_("Will need at least %.2f MB of disk space"), disk_mb);
	G_message(_("Will need at least %.2f MB of memory"), mem_mb);
	
    }
    else {
	double disk_mb, mem_mb;

	disk_mb = (double) nrows * ncols * 24. / 1048576.;
	mem_mb  = (double) srows * scols * 24. / 1048576. * segments_in_memory;
	mem_mb += nrows * ncols * 0.05 * 20. / 1048576.;    /* for Dijkstra search */
	G_message(_("Will need at least %.2f MB of disk space"), disk_mb);
	G_message(_("Will need at least %.2f MB of memory"), mem_mb);
    }
    G_message("--------------------------------------------");

    if (flag5->answer) {
	Rast_close(cost_fd);
	Rast_close(dtm_fd);
	exit(EXIT_SUCCESS);
    }

    /* Create segmented format file for cost layer and output layer */
    G_verbose_message(_("Creating some temporary files..."));

    in_fd = creat(in_file, 0600);
    if (segment_format(in_fd, nrows, ncols, srows, scols, sizeof(struct cc)) != 1)
    	G_fatal_error("can not create temporary file");

    close(in_fd);

    if (dir == 1) {
	dir_out_fd = creat(dir_out_file, 0600);
	if (segment_format(dir_out_fd, nrows, ncols, srows, scols,
		       sizeof(FCELL)) != 1)
	    G_fatal_error("can not create temporary file");
	close(dir_out_fd);
    }
    
    /* Open and initialize all segment files */
    in_fd = open(in_file, 2);
    if (segment_init(&cost_seg, in_fd, segments_in_memory) != 1)
    	G_fatal_error("can not initialize temporary file");

    if (dir == 1) {
	dir_out_fd = open(dir_out_file, 2);
	if (segment_init(&dir_seg, dir_out_fd, segments_in_memory) != 1)
	    G_fatal_error("can not initialize temporary file");
    }

    /* Write the dtm and cost layers in the segmented file */
    G_message(_("Reading raster maps <%s> and <%s>, initializing output..."),
	      G_fully_qualified_name(dtm_layer, dtm_mapset),
	      G_fully_qualified_name(cost_layer, cost_mapset));

    /* read required maps cost and dtm */
    {
	int skip_nulls;
	double p_dtm, p_cost;

	Rast_set_d_null_value(&dnullval, 1);
	costs.cost_out = dnullval;

	total_cells = nrows * ncols;

	skip_nulls = Rast_is_d_null_value(&null_cost);

	dtm_dsize = Rast_cell_size(dtm_data_type);
	cost_dsize = Rast_cell_size(cost_data_type);
	dtm_cell = Rast_allocate_buf(dtm_data_type);
	cost_cell = Rast_allocate_buf(cost_data_type);
	p_dtm = 0.0;
	p_cost = 0.0;

	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    Rast_get_row(dtm_fd, dtm_cell, row, dtm_data_type);
	    Rast_get_row(cost_fd, cost_cell, row, cost_data_type);
	    /* INPUT NULL VALUES: ??? */
	    ptr1 = cost_cell;
	    ptr2 = dtm_cell;

	    for (col = 0; col < ncols; col++) {
		if (Rast_is_null_value(ptr1, cost_data_type)) {
		    p_cost = null_cost;
		    if (skip_nulls) {
			total_cells--;
		    }
		}
		else {
		    switch (cost_data_type) {
		    case CELL_TYPE:
			p_cost = *(CELL *)ptr1;
			break;
		    case FCELL_TYPE:
			p_cost = *(FCELL *)ptr1;
			break;
		    case DCELL_TYPE:
			p_cost = *(DCELL *)ptr1;
			break;
		    }
		}
		costs.cost_in = p_cost;
		
		if (Rast_is_null_value(ptr2, dtm_data_type)) {
		    p_dtm = null_cost;
		    if (skip_nulls && !Rast_is_null_value(ptr1, cost_data_type)) {
			total_cells--;
		    }
		}
		else {
		    switch (dtm_data_type) {
		    case CELL_TYPE:
			p_dtm = *(CELL *)ptr2;
			break;
		    case FCELL_TYPE:
			p_dtm = *(FCELL *)ptr2;
			break;
		    case DCELL_TYPE:
			p_dtm = *(DCELL *)ptr2;
			break;
		    }
		}

		costs.dtm = p_dtm;
		segment_put(&cost_seg, &costs, row, col);
		ptr1 = G_incr_void_ptr(ptr1, cost_dsize);
		ptr2 = G_incr_void_ptr(ptr2, dtm_dsize);
	    }
	}
	G_free(dtm_cell);
	G_free(cost_cell);
	G_percent(1, 1, 1);
    }

    if (dir == 1) {
	G_message(_("Initializing directional output "));
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    for (col = 0; col < ncols; col++) {
		segment_put(&dir_seg, &dnullval, row, col);
	    }
	}
	G_percent(1, 1, 1);
    }

    /*   Scan the existing cum_cost_layer searching for starting points.
     *   Create a heap of starting points ordered by increasing costs.
     */
    init_heap();

    /* read vector with start points */
    if (opt7->answer) {
	struct Map_info In;
	struct line_pnts *Points;
	struct line_cats *Cats;
	struct bound_box box;
	struct start_pt *new_start_pt;
	int type, got_one = 0;

	G_message(_("Reading vector map <%s> with start points..."), opt7->answer);

	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();

	Vect_set_open_level(1); /* topology not required */

	if (1 > Vect_open_old(&In, opt7->answer, ""))
	    G_fatal_error(_("Unable to open vector map <%s>"), opt7->answer);

	Vect_rewind(&In);

	Vect_region_box(&window, &box);

	while (1) {
	    /* register line */
	    type = Vect_read_next_line(&In, Points, Cats);

	    /* Note: check for dead lines is not needed, because they are skipped by V1_read_next_line_nat() */
	    if (type == -1) {
		G_warning(_("Unable to read vector map"));
		continue;
	    }
	    else if (type == -2) {
		break;
	    }
	    if (!Vect_point_in_box(Points->x[0], Points->y[0], 0, &box))
		continue;
	    got_one = 1;

	    col = (int)Rast_easting_to_col(Points->x[0], &window);
	    row = (int)Rast_northing_to_row(Points->y[0], &window);

	    new_start_pt =
		(struct start_pt *)(G_malloc(sizeof(struct start_pt)));

	    new_start_pt->row = row;
	    new_start_pt->col = col;
	    new_start_pt->next = NULL;

	    if (head_start_pt == NULL) {
		head_start_pt = new_start_pt;
		pres_start_pt = new_start_pt;
		new_start_pt->next = NULL;
	    }
	    else {
		pres_start_pt->next = new_start_pt;
		pres_start_pt = new_start_pt;
	    }
	}

	Vect_close(&In);

	if (!got_one)
	    G_fatal_error(_("No start points found in vector <%s>"), opt7->answer);
    }

    /* read vector with stop points */
    if (opt8->answer) {
	struct Map_info In;
	struct line_pnts *Points;
	struct line_cats *Cats;
	struct bound_box box;
	struct start_pt *new_start_pt;
	int type;

	G_message(_("Reading vector map <%s> with stop points..."), opt8->answer);

	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();

	Vect_set_open_level(1); /* topology not required */

	if (1 > Vect_open_old(&In, opt8->answer, ""))
	    G_fatal_error(_("Unable to open vector map <%s>"), opt8->answer);

	Vect_rewind(&In);

	Vect_region_box(&window, &box);

	while (1) {
	    /* register line */
	    type = Vect_read_next_line(&In, Points, Cats);

	    /* Note: check for dead lines is not needed, because they are skipped by V1_read_next_line_nat() */
	    if (type == -1) {
		G_warning(_("Unable to read vector map"));
		continue;
	    }
	    else if (type == -2) {
		break;
	    }
	    if (!Vect_point_in_box(Points->x[0], Points->y[0], 0, &box))
		continue;
	    have_stop_points = 1;

	    col = (int)Rast_easting_to_col(Points->x[0], &window);
	    row = (int)Rast_northing_to_row(Points->y[0], &window);

	    new_start_pt =
		(struct start_pt *)(G_malloc(sizeof(struct start_pt)));

	    new_start_pt->row = row;
	    new_start_pt->col = col;
	    new_start_pt->next = NULL;

	    if (head_end_pt == NULL) {
		head_end_pt = new_start_pt;
		pres_stop_pt = new_start_pt;
		new_start_pt->next = NULL;
	    }
	    else {
		pres_stop_pt->next = new_start_pt;
		pres_stop_pt = new_start_pt;
	    }
	}

	Vect_close(&In);

	if (!have_stop_points)
	    G_fatal_error(_("No stop points found in vector <%s>"), opt8->answer);
    }

    /* read raster with start points */
    if (opt9->answer) {
	int dsize2;
	int fd;
	RASTER_MAP_TYPE data_type2;
	int got_one = 0;

	search_mapset = G_find_raster(opt9->answer, "");

	if (search_mapset == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), opt9->answer);

	fd = Rast_open_old(opt9->answer, "");
	data_type2 = Rast_get_map_type(fd);
	dsize2 = Rast_cell_size(data_type2);
	cell2 = Rast_allocate_buf(data_type2);
	if (!cell2)
	    G_fatal_error(_("Unable to allocate memory"));

	G_message(_("Reading %s... "), opt9->answer);
	for (row = 0; row < nrows; row++) {

	    G_percent(row, nrows, 2);
	    Rast_get_row(fd, cell2, row, data_type2);
	    ptr2 = cell2;
	    for (col = 0; col < ncols; col++) {
		/* Did I understand that concept of cummulative cost map? - (pmx) 12 april 2000 */
		if (!Rast_is_null_value(ptr2, data_type2)) {
		    double cellval;

		    segment_get(&cost_seg, &costs, row, col);

		    if (start_with_raster_vals == 1) {
			cellval = Rast_get_d_value(ptr2, data_type2);
			new_cell = insert(cellval, row, col);
			costs.cost_out = cellval;
			segment_put(&cost_seg, &costs, row, col);
		    }
		    else {
			value = &zero;
			new_cell = insert(zero, row, col);
			costs.cost_out = *value;
			segment_put(&cost_seg, &costs, row, col);
		    }
		    got_one = 1;
		}
		ptr2 = G_incr_void_ptr(ptr2, dsize2);
	    }
	}

	G_percent(1, 1, 1);

	Rast_close(fd);
	G_free(cell2);

	if (!got_one)
	    G_fatal_error(_("No start points found in raster <%s>"), opt9->answer);
    }

    /*  If the starting points are given on the command line start a linked
     *  list of cells ordered by increasing costs
     */
    if (head_start_pt) {
	struct start_pt *top_start_pt = NULL;

	top_start_pt = head_start_pt;
	while (top_start_pt != NULL) {
	    value = &zero;
	    if (top_start_pt->row < 0 || top_start_pt->row >= nrows
		|| top_start_pt->col < 0 || top_start_pt->col >= ncols)
		G_fatal_error(_("Specified starting location outside database window"));
	    new_cell = insert(zero, top_start_pt->row, top_start_pt->col);
	    segment_get(&cost_seg, &costs, top_start_pt->row,
			top_start_pt->col);
	    costs.cost_out = *value;
	    segment_put(&cost_seg, &costs, top_start_pt->row,
			top_start_pt->col);
	    top_start_pt = top_start_pt->next;
	}
    }

    /*  Loop through the heap and perform at each cell the following:
     *   1) If an adjacent cell has not already been assigned a value compute
     *      the min cost and assign it.
     *   2) Insert the adjacent cell in the heap.
     *   3) Free the memory allocated to the present cell.
     */

    G_debug(1, "total cells: %ld", total_cells);
    G_debug(1, "nrows x ncols: %d", nrows * ncols);
    G_message(_("Finding cost path"));
    n_processed = 0;

    pres_cell = get_lowest();
    while (pres_cell != NULL) {
	struct cost *ct;
	double N_dtm, NE_dtm, E_dtm, SE_dtm, S_dtm, SW_dtm, W_dtm, NW_dtm;
	double NNE_dtm, ENE_dtm, ESE_dtm, SSE_dtm, SSW_dtm, WSW_dtm, WNW_dtm,
	    NNW_dtm;
	double N_cost, NE_cost, E_cost, SE_cost, S_cost, SW_cost, W_cost,
	    NW_cost;
	double NNE_cost, ENE_cost, ESE_cost, SSE_cost, SSW_cost, WSW_cost,
	    WNW_cost, NNW_cost;

	N_dtm = NE_dtm = E_dtm = SE_dtm = S_dtm = SW_dtm = W_dtm = NW_dtm = dnullval;
	NNE_dtm = ENE_dtm = ESE_dtm = SSE_dtm = SSW_dtm = WSW_dtm = WNW_dtm = NNW_dtm = dnullval;

	N_cost = NE_cost = E_cost = SE_cost = S_cost = SW_cost = W_cost = NW_cost = dnullval;
	NNE_cost = ENE_cost = ESE_cost = SSE_cost = SSW_cost = WSW_cost = WNW_cost = NNW_cost = dnullval;

	/* If we have surpassed the user specified maximum cost, then quit */
	if (maxcost && ((double)maxcost < pres_cell->min_cost))
	    break;

	/* If I've already been updated, delete me */
	segment_get(&cost_seg, &costs, pres_cell->row, pres_cell->col);
	old_min_cost = costs.cost_out;
	if (!Rast_is_d_null_value(&old_min_cost)) {
	    if (pres_cell->min_cost > old_min_cost) {
		delete(pres_cell);
		pres_cell = get_lowest();
		continue;
	    }
	}

	my_dtm = costs.dtm;
	if (Rast_is_d_null_value(&my_dtm)) {
	    delete(pres_cell);
	    pres_cell = get_lowest();
	    continue;
	}
	my_cost = costs.cost_in;
	if (Rast_is_d_null_value(&my_cost)) {
	    delete(pres_cell);
	    pres_cell = get_lowest();
	    continue;
	}

	G_percent(n_processed++, total_cells, 1);

	/*          9    10       Order in which neighbors 
	 *       13 5  3  6 14    are visited (Knight move).
	 *          1     2
	 *       16 8  4  7 15
	 *         12    11
	 */

	/* drainage directions in degrees CCW from East
	 * drainage directions are set for each neighbor and must be 
	 * read as from neighbor to current cell
	 * 
	 * X = neighbor:
	 * 
	 *       112.5       67.5 
	 * 157.5 135    90   45   22.5
	 *       180     X  360
	 * 202.5 225   270  315   337.5
	 *       247.5      292.5
	 * 
	 * X = present cell, directions for neighbors:
	 * 
	 *       292.5      247.5 
	 * 337.5 315   270  225    202.5
	 *       360     X  180
	 *  22.5  45    90  135    157.5
	 *        67.5      112.5
	 */

	for (neighbor = 1; neighbor <= total_reviewed; neighbor++) {
	    switch (neighbor) {
	    case 1:
		col = pres_cell->col - 1;
		cur_dir = 360.0;
		break;
	    case 2:
		col = pres_cell->col + 1;
		cur_dir = 180.0;
		break;
	    case 3:
		row = pres_cell->row - 1;
		col = pres_cell->col;
		cur_dir = 270.0;
		break;
	    case 4:
		row = pres_cell->row + 1;
		cur_dir = 90.0;
		break;
	    case 5:
		row = pres_cell->row - 1;
		col = pres_cell->col - 1;
		cur_dir = 315.0;
		break;
	    case 6:
		col = pres_cell->col + 1;
		cur_dir = 225.0;
		break;
	    case 7:
		row = pres_cell->row + 1;
		cur_dir = 135.0;
		break;
	    case 8:
		col = pres_cell->col - 1;
		cur_dir = 45.0;
		break;
	    case 9:
		row = pres_cell->row - 2;
		col = pres_cell->col - 1;
		cur_dir = 292.5;
		break;
	    case 10:
		col = pres_cell->col + 1;
		cur_dir = 247.5;
		break;
	    case 11:
		row = pres_cell->row + 2;
		cur_dir = 112.5;
		break;
	    case 12:
		col = pres_cell->col - 1;
		cur_dir = 67.5;
		break;
	    case 13:
		row = pres_cell->row - 1;
		col = pres_cell->col - 2;
		cur_dir = 337.5;
		break;
	    case 14:
		col = pres_cell->col + 2;
		cur_dir = 202.5;
		break;
	    case 15:
		row = pres_cell->row + 1;
		cur_dir = 157.5;
		break;
	    case 16:
		col = pres_cell->col - 2;
		cur_dir = 22.5;
		break;
	    }

	    if (row < 0 || row >= nrows)
		continue;
	    if (col < 0 || col >= ncols)
		continue;

	    min_cost = dnullval;
	    segment_get(&cost_seg, &costs, row, col);
	    switch (neighbor) {
	    case 1:
		W_dtm = costs.dtm;
		W_cost = costs.cost_in;
		if (Rast_is_d_null_value(&W_cost))
		    continue;
		check_dtm = (W_dtm - my_dtm) / EW_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(W_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(W_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(W_dtm - my_dtm) * c;
		fcost_cost = (double)(W_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (EW_fac * a) +
		    lambda * fcost_cost * EW_fac;
		break;
	    case 2:
		E_dtm = costs.dtm;
		E_cost = costs.cost_in;
		if (Rast_is_d_null_value(&E_cost))
		    continue;
		check_dtm = (E_dtm - my_dtm) / EW_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(E_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(E_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(E_dtm - my_dtm) * c;
		fcost_cost = (double)(E_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (EW_fac * a) +
		    lambda * fcost_cost * EW_fac;
		break;
	    case 3:
		N_dtm = costs.dtm;
		N_cost = costs.cost_in;
		if (Rast_is_d_null_value(&N_cost))
		    continue;
		check_dtm = (N_dtm - my_dtm) / NS_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(N_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(N_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(N_dtm - my_dtm) * c;
		fcost_cost = (double)(N_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (NS_fac * a) +
		    lambda * fcost_cost * NS_fac;
		break;
	    case 4:
		S_dtm = costs.dtm;
		S_cost = costs.cost_in;
		if (Rast_is_d_null_value(&S_cost))
		    continue;
		check_dtm = (S_dtm - my_dtm) / NS_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(S_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(S_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(S_dtm - my_dtm) * c;
		fcost_cost = (double)(S_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (NS_fac * a) +
		    lambda * fcost_cost * NS_fac;
		break;
	    case 5:
		NW_dtm = costs.dtm;
		NW_cost = costs.cost_in;
		if (Rast_is_d_null_value(&NW_cost))
		    continue;
		check_dtm = (NW_dtm - my_dtm) / DIAG_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(NW_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(NW_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(NW_dtm - my_dtm) * c;
		fcost_cost = (double)(NW_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (DIAG_fac * a) +
		    lambda * fcost_cost * DIAG_fac;
		break;
	    case 6:
		NE_dtm = costs.dtm;
		NE_cost = costs.cost_in;
		if (Rast_is_d_null_value(&NE_cost))
		    continue;
		check_dtm = (NE_dtm - my_dtm) / DIAG_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(NE_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(NE_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(NE_dtm - my_dtm) * c;
		fcost_cost = (double)(NE_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (DIAG_fac * a) +
		    lambda * fcost_cost * DIAG_fac;
		break;
	    case 7:
		SE_dtm = costs.dtm;
		SE_cost = costs.cost_in;
		if (Rast_is_d_null_value(&SE_cost))
		    continue;
		check_dtm = (SE_dtm - my_dtm) / DIAG_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(SE_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(SE_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(SE_dtm - my_dtm) * c;
		fcost_cost = (double)(SE_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (DIAG_fac * a) +
		    lambda * fcost_cost * DIAG_fac;
		break;
	    case 8:
		SW_dtm = costs.dtm;
		SW_cost = costs.cost_in;
		if (Rast_is_d_null_value(&SW_cost))
		    continue;
		check_dtm = (SW_dtm - my_dtm) / DIAG_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(SW_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(SW_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(SW_dtm - my_dtm) * c;
		fcost_cost = (double)(SW_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (DIAG_fac * a) +
		    lambda * fcost_cost * DIAG_fac;
		break;
	    case 9:
		NNW_dtm = costs.dtm;
		NNW_cost = costs.cost_in;
		if (Rast_is_d_null_value(&NNW_cost))
		    continue;
		check_dtm = (NNW_dtm - my_dtm) / V_DIAG_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(NNW_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(NNW_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(NNW_dtm - my_dtm) * c;
		fcost_cost =
		    (double)(N_cost + NW_cost + NNW_cost + my_cost) / 4.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (V_DIAG_fac * a) +
		    lambda * fcost_cost * V_DIAG_fac;
		break;
	    case 10:
		NNE_dtm = costs.dtm;
		NNE_cost = costs.cost_in;
		if (Rast_is_d_null_value(&NNE_cost))
		    continue;
		check_dtm = ((NNE_dtm - my_dtm) / V_DIAG_fac);
		if (check_dtm >= 0)
		    fcost_dtm = (double)(NNE_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(NNE_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(NNE_dtm - my_dtm) * c;
		fcost_cost =
		    (double)(N_cost + NE_cost + NNE_cost + my_cost) / 4.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (V_DIAG_fac * a) +
		    lambda * fcost_cost * V_DIAG_fac;
		break;
	    case 11:
		SSE_dtm = costs.dtm;
		SSE_cost = costs.cost_in;
		if (Rast_is_d_null_value(&SSE_cost))
		    continue;
		check_dtm = (SSE_dtm - my_dtm) / V_DIAG_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(SSE_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(SSE_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(SSE_dtm - my_dtm) * c;
		fcost_cost =
		    (double)(S_cost + SE_cost + SSE_cost + my_cost) / 4.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (V_DIAG_fac * a) +
		    lambda * fcost_cost * V_DIAG_fac;
		break;
	    case 12:
		SSW_dtm = costs.dtm;
		SSW_cost = costs.cost_in;
		if (Rast_is_d_null_value(&SSW_cost))
		    continue;
		check_dtm = (SSW_dtm - my_dtm) / V_DIAG_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(SSW_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(SSW_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(SSW_dtm - my_dtm) * c;
		fcost_cost =
		    (double)(S_cost + SW_cost +	SSW_cost + my_cost) / 4.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (V_DIAG_fac * a) +
		    lambda * fcost_cost * V_DIAG_fac;
		break;
	    case 13:
		WNW_dtm = costs.dtm;
		WNW_cost = costs.cost_in;
		if (Rast_is_d_null_value(&WNW_cost))
		    continue;
		check_dtm = (WNW_dtm - my_dtm) / H_DIAG_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(WNW_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(WNW_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(WNW_dtm - my_dtm) * c;
		fcost_cost =
		    (double)(W_cost + NW_cost + WNW_cost + my_cost) / 4.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (H_DIAG_fac * a) +
		    lambda * fcost_cost * H_DIAG_fac;
		break;
	    case 14:
		ENE_dtm = costs.dtm;
		ENE_cost = costs.cost_in;
		if (Rast_is_d_null_value(&ENE_cost))
		    continue;
		check_dtm = (ENE_dtm - my_dtm) / H_DIAG_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(ENE_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(ENE_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(ENE_dtm - my_dtm) * c;
		fcost_cost =
		    (double)(E_cost + NE_cost + ENE_cost + my_cost) / 4.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (H_DIAG_fac * a) +
		    lambda * fcost_cost * H_DIAG_fac;
		break;
	    case 15:
		ESE_dtm = costs.dtm;
		ESE_cost = costs.cost_in;
		if (Rast_is_d_null_value(&ESE_cost))
		    continue;
		check_dtm = (ESE_dtm - my_dtm) / H_DIAG_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(ESE_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(ESE_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(ESE_dtm - my_dtm) * c;
		fcost_cost =
		    (double)(E_cost + SE_cost + ESE_cost + my_cost) / 4.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (H_DIAG_fac * a) +
		    lambda * fcost_cost * H_DIAG_fac;
		break;
	    case 16:
		WSW_dtm = costs.dtm;
		WSW_cost = costs.cost_in;
		if (Rast_is_d_null_value(&WSW_cost))
		    continue;
		check_dtm = (WSW_dtm - my_dtm) / H_DIAG_fac;
		if (check_dtm >= 0)
		    fcost_dtm = (double)(WSW_dtm - my_dtm) * b;
		else if (check_dtm < (slope_factor))
		    fcost_dtm = (double)(WSW_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(WSW_dtm - my_dtm) * c;
		fcost_cost =
		    (double)(W_cost + SW_cost +	WSW_cost + my_cost) / 4.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (H_DIAG_fac * a) +
		    lambda * fcost_cost * H_DIAG_fac;
		break;
	    }

	    if (Rast_is_d_null_value(&min_cost))
		continue;

	    segment_get(&cost_seg, &costs, row, col);
	    old_min_cost = costs.cost_out;

	    if (Rast_is_d_null_value(&old_min_cost)) {
		costs.cost_out = min_cost;
		segment_put(&cost_seg, &costs, row, col);
		new_cell = insert(min_cost, row, col);
		if (dir == 1)
		    segment_put(&dir_seg, &cur_dir, row, col);
	    }
	    else if (old_min_cost > min_cost) {
		costs.cost_out = min_cost;
		segment_put(&cost_seg, &costs, row, col);
		new_cell = insert(min_cost, row, col);
		if (dir == 1)
		    segment_put(&dir_seg, &cur_dir, row, col);
	    }
	}

	if (have_stop_points && time_to_stop(pres_cell->row, pres_cell->col))
	    break;

	ct = pres_cell;
	delete(pres_cell);
	pres_cell = get_lowest();
	if (ct == pres_cell)
	    G_warning(_("Error, ct == pres_cell"));
    }
    G_percent(1, 1, 1);

    /* free heap */
    free_heap();
    
    /* Open cumulative cost layer for writing */
    cum_fd = Rast_open_new(cum_cost_layer, cum_data_type);
    cum_cell = Rast_allocate_buf(cum_data_type);

    /* Copy segmented map to output map */
    G_message(_("Writing output raster map %s... "), cum_cost_layer);

    cell2 = Rast_allocate_buf(dtm_data_type);
    {
	void *p;
	void *p2;
	int cum_dsize = Rast_cell_size(cum_data_type);

	Rast_set_null_value(cell2, ncols, dtm_data_type);

	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    if (keep_nulls)
		Rast_get_row(dtm_fd, cell2, row, dtm_data_type);

	    p = cum_cell;
	    p2 = cell2;
	    for (col = 0; col < ncols; col++) {
		if (keep_nulls) {
		    if (Rast_is_null_value(p2, dtm_data_type)) {
			Rast_set_null_value(p, 1, cum_data_type);
			p = G_incr_void_ptr(p, cum_dsize);
			p2 = G_incr_void_ptr(p2, dtm_dsize);
			continue;
		    }
		}
		segment_get(&cost_seg, &costs, row, col);
		min_cost = costs.cost_out;
		if (Rast_is_d_null_value(&min_cost)) {
		    Rast_set_null_value((p), 1, cum_data_type);
		}
		else {
		    if (min_cost > peak)
			peak = min_cost;

		    switch (cum_data_type) {
		    case CELL_TYPE:
			*(CELL *)p = (CELL)(min_cost + .5);
			break;
		    case FCELL_TYPE:
			*(FCELL *)p = (FCELL)(min_cost);
			break;
		    case DCELL_TYPE:
			*(DCELL *)p = (DCELL)(min_cost);
			break;
		    }
		}
		p = G_incr_void_ptr(p, cum_dsize);
		p2 = G_incr_void_ptr(p2, dtm_dsize);
	    }
	    Rast_put_row(cum_fd, cum_cell, cum_data_type);
	}
	G_percent(1, 1, 1);
	G_free(cum_cell);
	G_free(cell2);
    }

    if (dir == 1) {
	void *p;
	size_t dir_size = Rast_cell_size(dir_data_type);

	dir_fd = Rast_open_new(move_dir_layer, dir_data_type);
	dir_cell = Rast_allocate_buf(dir_data_type);

	G_message(_("Writing movement direction file %s..."), move_dir_layer);
	for (row = 0; row < nrows; row++) {
	    p = dir_cell;
	    for (col = 0; col < ncols; col++) {
		segment_get(&dir_seg, &cur_dir, row, col);
		*((FCELL *) p) = cur_dir;
		p = G_incr_void_ptr(p, dir_size);
	    }
	    Rast_put_row(dir_fd, dir_cell, dir_data_type);
	}
	G_percent(1, 1, 1);
	G_free(dir_cell);
    }

    segment_release(&cost_seg);	/* release memory  */
    if (dir == 1)
	segment_release(&dir_seg);
    Rast_close(dtm_fd);
    Rast_close(cost_fd);
    Rast_close(cum_fd);
    if (dir == 1)
	Rast_close(dir_fd);
    close(in_fd);		/* close all files */
    if (dir == 1)
	close(dir_out_fd);
    unlink(in_file);	/* remove submatrix files  */
    if (dir == 1)
	unlink(dir_out_file);

    /* writing history file */
    Rast_short_history(cum_cost_layer, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(cum_cost_layer, &history);

    if (dir == 1) {
	Rast_short_history(move_dir_layer, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(move_dir_layer, &history);
    }

    /* Create colours for output map */

    /*
     * Rast_read_range (cum_cost_layer, "", &range);
     * Rast_get_range_min_max(&range, &min, &max);
     * G_make_color_wave(&colors,min, max);
     * Rast_write_colors (cum_cost_layer,"",&colors);
     */

    G_done_msg(_("Peak cost value: %f."), peak);

    exit(EXIT_SUCCESS);
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int
process_answers(char **answers, struct start_pt **points,
		struct start_pt **top_start_pt)
{
    int col, row, n;
    double east, north;

    struct start_pt *new_start_pt;
    int got_one = 0;

    *points = NULL;

    if (!answers)
	return (0);

    for (n = 0; *answers != NULL; answers += 2) {
	if (!G_scan_easting(*answers, &east, G_projection()))
	    G_fatal_error(_("Illegal x coordinate <%s>"), *answers);
	if (!G_scan_northing(*(answers + 1), &north, G_projection()))
	    G_fatal_error(_("Illegal y coordinate <%s>"), *(answers + 1));

	if (east < window.west || east > window.east ||
	    north < window.south || north > window.north) {
	    G_warning(_("Warning, ignoring point outside window: %.4f,%.4f"),
		      east, north);
	    continue;
	}
	else
	    got_one = 1;

	row = (window.north - north) / window.ns_res;
	col = (east - window.west) / window.ew_res;

	new_start_pt = (struct start_pt *)(G_malloc(sizeof(struct start_pt)));

	new_start_pt->row = row;
	new_start_pt->col = col;
	new_start_pt->next = NULL;

	if (*points == NULL) {
	    *points = new_start_pt;
	    *top_start_pt = new_start_pt;
	    new_start_pt->next = NULL;
	}
	else {
	    (*top_start_pt)->next = new_start_pt;
	    *top_start_pt = new_start_pt;
	}
    }
    return (got_one);
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int time_to_stop(int row, int col)
{
    static int total = 0;
    static int hits = 0;
    struct start_pt *points;

    if (total == 0) {
	for (points = head_end_pt;
	     points != NULL; points = points->next, total++) ;
    }

    for (points = head_end_pt; points != NULL; points = points->next)

	if (points->row == row && points->col == col) {
	    hits++;
	    if (hits == total)
		return (1);
	}

    return (0);
}
