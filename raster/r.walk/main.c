
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
#include <grass/site.h>
#include <grass/segment.h>
#include "cost.h"
#include "stash.h"
#include "local_proto.h"
#include <grass/glocale.h>

struct variables
{
    char *alias;
    int position;
} variables[] = {
    {"output", CUM_COST_LAYER},
    {"input", COST_LAYER},
    {"coor", START_PT}
};

struct start_pt *head_start_pt = NULL;
struct start_pt *head_end_pt = NULL;

struct Cell_head window;


/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int main(int argc, char *argv[])
{
    const char *cum_cost_layer;
    const char *start_layer, *cost_layer, *dtm_layer;
    void *dtm_cell, *cost_cell, *cum_cell, *cell2 = NULL;
    SEGMENT dtm_in_seg, cost_in_seg, out_seg;
    char *dtm_in_file, *cost_in_file, *out_file;
    double *dtm_value, *cost_value, *value_start_pt;
    char buf[400];
    extern struct Cell_head window;
    double NS_fac, EW_fac, DIAG_fac, H_DIAG_fac, V_DIAG_fac;
    double fcost_dtm, fcost_cost;
    double min_cost, old_min_cost;
    double zero = 0.0;
    int at_percent = 0;
    int col = 0, row = 0, nrows = 0, ncols = 0;
    int maxcost, par_number;
    int maxmem;
    int nseg;
    int cost_fd, cum_fd, dtm_fd;
    int have_start_points;
    int have_stop_points;
    int dtm_in_fd, cost_in_fd, out_fd;
    double my_dtm, my_cost;
    double null_cost;
    double a, b, c, d, lambda, slope_factor;
    int srows, scols;
    int total_reviewed;
    int keep_nulls = 1;
    int start_with_raster_vals = 1;
    int neighbor;
    int segments_in_memory;
    long n_processed = 0;
    long total_cells;
    struct GModule *module;
    struct Flag *flag2, *flag3, *flag4;
    struct Option *opt1, *opt2, *opt3, *opt4, *opt5, *opt6, *opt7, *opt8;
    struct Option *opt9, *opt10, *opt11, *opt12, *opt13, *opt14;
    struct cost *pres_cell, *new_cell;
    struct History history;
    struct start_pt *pres_start_pt = NULL;
    struct start_pt *pres_stop_pt = NULL;

    void *ptr2;
    RASTER_MAP_TYPE dtm_data_type, data_type2, cost_data_type, cum_data_type =
	DCELL_TYPE, cat;
    double peak = 0.0;
    int dtm_dsize, cost_dsize;

    /* Definition for dimension and region check */
    struct Cell_head dtm_cellhd, cost_cellhd;
    int dtm_head_ok, cost_head_ok;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Outputs a raster map layer showing the "
	  "anisotropic cumulative cost of moving between different "
	  "geographic locations on an input elevation raster map "
	  "layer whose cell category values represent elevation "
	  "combined with an input raster map layer whose cell "
	  "values represent friction cost.");

    opt2 = G_define_option();
    opt2->key = "elevation";
    opt2->type = TYPE_STRING;
    opt2->required = YES;
    opt2->gisprompt = "old,cell,raster";
    opt2->description = _("Name of elevation input raster map");

    opt12 = G_define_option();
    opt12->key = "friction";
    opt12->type = TYPE_STRING;
    opt12->required = YES;
    opt12->gisprompt = "old,cell,raster";
    opt12->description =
	_("Name of input raster map containing friction costs");

    opt1 = G_define_option();
    opt1->key = "output";
    opt1->type = TYPE_STRING;
    opt1->required = YES;
    opt1->gisprompt = "new,cell,raster";
    opt1->description = _("Name of raster map to contain results");

    opt14 = G_define_option();
    opt14->key = "start_map";
    opt14->type = TYPE_STRING;
    opt14->required = NO;
    opt14->gisprompt = "old,cell,raster";
    opt14->description =
	_("Name of input raster map containing starting points");

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

    opt9 = G_define_option();
    opt9->key = "percent_memory";
    opt9->type = TYPE_INTEGER;
    opt9->required = NO;
    opt9->multiple = NO;
    opt9->answer = "100";
    opt9->description = _("Percent of map to keep in memory");

    opt14 = G_define_option();
    opt14->key = "nseg";
    opt14->type = TYPE_INTEGER;
    opt14->required = NO;
    opt14->multiple = NO;
    opt14->answer = "4";
    opt14->description =
	_("Number of the segment to create (segment library)");

    opt10 = G_define_option();
    opt10->key = "walk_coeff";
    opt10->type = TYPE_DOUBLE;
    opt10->key_desc = "a,b,c,d";
    opt10->required = NO;
    opt10->multiple = NO;
    opt10->answer = "0.72,6.0,1.9998,-1.9998";
    opt10->description =
	_("Coefficients for walking energy formula parameters a,b,c,d");

    opt11 = G_define_option();
    opt11->key = "lambda";
    opt11->type = TYPE_DOUBLE;
    opt11->required = NO;
    opt11->multiple = NO;
    opt11->answer = "1.0";
    opt11->description =
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

    /*   Parse command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Initalize access to database and create temporary files */

    dtm_in_file = G_tempfile();
    cost_in_file = G_tempfile();
    out_file = G_tempfile();

    /*  Get database window parameters      */

    G_get_window(&window);

    /*  Find north-south, east_west and diagonal factors */

    EW_fac = window.ew_res;	/* Must be the physical distance */
    NS_fac = window.ns_res;

    DIAG_fac = (double)sqrt((double)(NS_fac * NS_fac + EW_fac * EW_fac));
    V_DIAG_fac =
	(double)sqrt((double)(4 * NS_fac * NS_fac + EW_fac * EW_fac));
    H_DIAG_fac =
	(double)sqrt((double)(NS_fac * NS_fac + 4 * EW_fac * EW_fac));

    G_set_d_null_value(&null_cost, 1);

    if (flag2->answer)
	total_reviewed = 16;
    else
	total_reviewed = 8;

    keep_nulls = flag3->answer;

    have_start_points =
	process_answers(opt3->answers, &head_start_pt, &pres_start_pt);

    have_stop_points =
	process_answers(opt4->answers, &head_end_pt, &pres_stop_pt);

    if (sscanf(opt5->answer, "%d", &maxcost) != 1 || maxcost < 0)
	G_fatal_error(_("Inappropriate maximum cost: %d"), maxcost);

    if (sscanf(opt9->answer, "%d", &maxmem) != 1 || maxmem < 0 ||
	maxmem > 100)
	G_fatal_error(_("Inappropriate percent memory: %d"), maxmem);

    /* Getting walking energy formula parameters */
    if ((par_number =
	 sscanf(opt10->answer, "%lf,%lf,%lf,%lf", &a, &b, &c, &d)) != 4)
	G_fatal_error(_("Missing required value: got %d instead of 4"),
		      par_number);
    else {

	G_message(_("Walking costs are a=%lf b=%lf c=%lf d=%lf"), a, b, c, d);
    }

    /* Getting  lambda */
    if ((par_number = sscanf(opt11->answer, "%lf", &lambda)) != 1)
	G_fatal_error(_("Missing required value: %d"), par_number);
    else {

	G_message(_("Lambda is %lf"), lambda);
    }

    /*Getting  slope_factor */
    if ((par_number = sscanf(opt13->answer, "%lf", &slope_factor)) != 1)
	G_fatal_error(_("Missing required value: %d"), par_number);
    else {

	G_message(_("Slope_factor is %lf"), slope_factor);
    }

    if ((par_number = sscanf(opt14->answer, "%d", &nseg)) != 1)
	G_fatal_error(_("Missing required value: %d"), par_number);
    else {

	G_message(_("Nseg is %d"), nseg);
    }

    if ((opt6->answer == NULL) ||
	(sscanf(opt6->answer, "%lf", &null_cost) != 1)) {

	G_message(_("Null cells excluded from cost evaluation."));
	G_set_d_null_value(&null_cost, 1);
    }
    else if (keep_nulls)
	G_message(_("Input null cell will be retained into output map"));


    if (opt7->answer) {
	struct Map_info *fp;
	struct start_pt *new_start_pt;
	Site *site = NULL;	/* pointer to Site */
	int dims, strs, dbls;

	fp = G_fopen_sites_old(opt7->answer, "");

	if (G_site_describe(fp, &dims, &cat, &strs, &dbls))
	    G_fatal_error("Failed to guess site file format");
	site = G_site_new_struct(cat, dims, strs, dbls);

	for (; (G_site_get(fp, site) != EOF);) {
	    if (!G_site_in_region(site, &window))
		continue;
	    have_start_points = 1;

	    col = (int)G_easting_to_col(site->east, &window);
	    row = (int)G_northing_to_row(site->north, &window);

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

	G_site_free_struct(site);
	G_sites_close(fp);
    }

    if (opt8->answer) {
	struct Map_info *fp;
	struct start_pt *new_start_pt;
	Site *site = NULL;	/* pointer to Site */
	int dims, strs, dbls;

	fp = G_fopen_sites_old(opt8->answer, "");

	if (G_site_describe(fp, &dims, &cat, &strs, &dbls))
	    G_fatal_error("Failed to guess site file format\n");
	site = G_site_new_struct(cat, dims, strs, dbls);

	for (; (G_site_get(fp, site) != EOF);) {
	    if (!G_site_in_region(site, &window))
		continue;
	    have_stop_points = 1;

	    col = (int)G_easting_to_col(site->east, &window);
	    row = (int)G_northing_to_row(site->north, &window);

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

	G_site_free_struct(site);
	G_sites_close(fp);
    }

    if (!G_is_d_null_value(&null_cost)) {
	if (null_cost < 0.0) {
	    G_warning(_("Warning: assigning negative cost to null cell. Null cells excluded."));
	    G_set_d_null_value(&null_cost, 1);
	}
    }
    else {
	keep_nulls = 0;		/* handled automagically... */
    }

    start_layer = opt14->answer;
    dtm_layer = opt2->answer;
    cost_layer = opt12->answer;
    cum_cost_layer = opt1->answer;

    /*  Find number of rows and columns in window    */

    nrows = G_window_rows();
    ncols = G_window_cols();


    /*  Open cost cell layer for reading  */

    dtm_fd = G_open_cell_old(dtm_layer, "");
    cost_fd = G_open_cell_old(cost_layer, "");

    if (dtm_fd < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), dtm_layer);

    if (cost_fd < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), cost_layer);

    dtm_head_ok = G_get_cellhd(dtm_layer, "", &dtm_cellhd) >= 0;
    cost_head_ok = G_get_cellhd(cost_layer, "", &cost_cellhd) >= 0;

    /*Reading headers from maps */

    if (!dtm_head_ok)
	G_fatal_error(_("Unable to read %s"), dtm_layer);
    if (!cost_head_ok)
	G_fatal_error(_("Unable to read %s"), cost_layer);

    /*Projection */

    if (dtm_cellhd.proj != cost_cellhd.proj)
	G_fatal_error(_("Map with different projection"));

    dtm_data_type = G_get_raster_map_type(dtm_fd);
    cost_data_type = G_get_raster_map_type(cost_fd);
    dtm_cell = G_allocate_raster_buf(dtm_data_type);
    cost_cell = G_allocate_raster_buf(cost_data_type);

    /*   Parameters for map submatrices   */


    switch (dtm_data_type) {
    case (CELL_TYPE):
	G_message(_("DTM_Source map is: Integer cell type"));
	break;
    case (FCELL_TYPE):
	G_message(_("DTM_Source map is: Floating point (float) cell type"));
	break;
    case (DCELL_TYPE):
	G_message(_("DTM_Source map is: Floating point (double) cell type"));
	break;
    }
    G_message(_(" %d rows, %d cols"), dtm_cellhd.rows, dtm_cellhd.cols);



    switch (cost_data_type) {
    case (CELL_TYPE):
	G_message(_("COST_Source map is: Integer cell type"));
	break;
    case (FCELL_TYPE):
	G_message(_("COST_Source map is: Floating point (float) cell type"));
	break;
    case (DCELL_TYPE):
	G_message(_("COST_Source map is: Floating point (double) cell type"));
	break;
    }
    G_message(_(" %d rows, %d cols"), cost_cellhd.rows, cost_cellhd.cols);

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
	G_message(_("Output map is: Integer cell type"));
	break;
    case (FCELL_TYPE):
	G_message(_("Output map is: Floating point (float) cell type"));
	break;
    case (DCELL_TYPE):
	G_message(_("Output map is: Floating point (double) cell type"));
	break;
    }
    G_message(_(" %d rows, %d cols"), nrows, ncols);
    G_format_resolution(window.ew_res, buf, window.proj);
    G_message(_(" EW resolution %s (%lf)"), buf, window.ew_res);
    G_format_resolution(window.ns_res, buf, window.proj);
    G_message(_(" NS resolution %s (%lf)"), buf, window.ns_res);


    srows = nrows / nseg + 1;
    scols = ncols / nseg + 1;
    if (maxmem > 0)
	segments_in_memory =
	    2 + maxmem * (nrows / srows) * (ncols / scols) / 100;
    else
	segments_in_memory = 4 * (nrows / srows + ncols / scols + 2);

    /*   Create segmented format files for cost layer and output layer  */


    G_message(_("Creating some temporary files..."));

    dtm_in_fd = creat(dtm_in_file, 0600);
    segment_format(dtm_in_fd, nrows, ncols, srows, scols, sizeof(double));
    close(dtm_in_fd);

    cost_in_fd = creat(cost_in_file, 0600);
    segment_format(cost_in_fd, nrows, ncols, srows, scols, sizeof(double));
    close(cost_in_fd);

    out_fd = creat(out_file, 0600);
    segment_format(out_fd, nrows, ncols, srows, scols, sizeof(double));
    close(out_fd);

    /*   Open initialize and segment all files  */

    dtm_in_fd = open(dtm_in_file, 2);
    segment_init(&dtm_in_seg, dtm_in_fd, segments_in_memory);

    cost_in_fd = open(cost_in_file, 2);
    segment_init(&cost_in_seg, cost_in_fd, segments_in_memory);

    out_fd = open(out_file, 2);
    segment_init(&out_seg, out_fd, segments_in_memory);

    /*   Write the cost layer in the segmented file  */


    G_message(_("Reading %s..."), dtm_layer);

    start_with_raster_vals = flag4->answer;

    {
	int i;
	double p;

	dtm_dsize = G_raster_size(dtm_data_type);
	p = 0.0;

	for (row = 0; row < nrows; row++) {

	    G_percent(row, nrows, 2);
	    if (G_get_raster_row(dtm_fd, dtm_cell, row, dtm_data_type) < 0)
		G_fatal_error(_("Unable to read raster map <%s> row %d"),
			      dtm_layer, row);
	    /* INPUT NULL VALUES: ??? */
	    ptr2 = dtm_cell;
	    switch (dtm_data_type) {
	    case CELL_TYPE:
		for (i = 0; i < ncols; i++) {
		    if (G_is_null_value(ptr2, dtm_data_type)) {
			p = null_cost;
		    }
		    else {
			p = *(int *)ptr2;
		    }
		    segment_put(&dtm_in_seg, &p, row, i);
		    ptr2 = G_incr_void_ptr(ptr2, dtm_dsize);
		}
		break;
	    case FCELL_TYPE:
		for (i = 0; i < ncols; i++) {
		    if (G_is_null_value(ptr2, dtm_data_type)) {
			p = null_cost;
		    }
		    else {
			p = *(float *)ptr2;
		    }
		    segment_put(&dtm_in_seg, &p, row, i);
		    ptr2 = G_incr_void_ptr(ptr2, dtm_dsize);
		}
		break;

	    case DCELL_TYPE:
		for (i = 0; i < ncols; i++) {
		    if (G_is_null_value(ptr2, dtm_data_type)) {
			p = null_cost;
		    }
		    else {
			p = *(double *)ptr2;
		    }
		    segment_put(&dtm_in_seg, &p, row, i);
		    ptr2 = G_incr_void_ptr(ptr2, dtm_dsize);
		}
		break;
	    }
	}
    }


    G_message(_("Reading %s..."), cost_layer);

    {
	int i;
	double p;

	cost_dsize = G_raster_size(cost_data_type);
	p = 0.0;
	for (row = 0; row < nrows; row++) {

	    G_percent(row, nrows, 2);
	    if (G_get_raster_row(cost_fd, cost_cell, row, cost_data_type) < 0)
		G_fatal_error(_("Unable to read raster map <%s> row %d"),
			      cost_layer, row);
	    /* INPUT NULL VALUES: ??? */
	    ptr2 = cost_cell;
	    switch (cost_data_type) {
	    case CELL_TYPE:
		for (i = 0; i < ncols; i++) {
		    if (G_is_null_value(ptr2, cost_data_type)) {
			p = null_cost;
		    }
		    else {
			p = *(int *)ptr2;
		    }
		    segment_put(&cost_in_seg, &p, row, i);
		    ptr2 = G_incr_void_ptr(ptr2, cost_dsize);
		}
		break;
	    case FCELL_TYPE:
		for (i = 0; i < ncols; i++) {
		    if (G_is_null_value(ptr2, cost_data_type)) {
			p = null_cost;
		    }
		    else {
			p = *(float *)ptr2;
		    }
		    segment_put(&cost_in_seg, &p, row, i);
		    ptr2 = G_incr_void_ptr(ptr2, cost_dsize);
		}
		break;

	    case DCELL_TYPE:
		for (i = 0; i < ncols; i++) {
		    if (G_is_null_value(ptr2, cost_data_type)) {
			p = null_cost;
		    }
		    else {
			p = *(double *)ptr2;
		    }
		    segment_put(&cost_in_seg, &p, row, i);
		    ptr2 = G_incr_void_ptr(ptr2, cost_dsize);
		}
		break;
	    }
	}
    }

    segment_flush(&dtm_in_seg);
    segment_flush(&cost_in_seg);


    G_percent(row, nrows, 2);

    /* Initialize output map with NULL VALUES */

    /*   Initialize segmented output file  */

    G_message(_("Initializing output "));
    {
	double *fbuff;
	int i;

	fbuff = (double *)G_malloc((unsigned int)(ncols * sizeof(double)));

	if (fbuff == NULL)
	    G_fatal_error(_("Unable to allocate memory for segment fbuff == NULL"));

	G_set_d_null_value(fbuff, ncols);

	for (row = 0; row < nrows; row++) {
	    {
		G_percent(row, nrows, 2);
	    }
	    for (i = 0; i < ncols; i++) {
		segment_put(&out_seg, &fbuff[i], row, i);
	    }

	}
	segment_flush(&out_seg);

	G_percent(row, nrows, 2);
	G_free(fbuff);
    }

    /*   Scan the existing cum_cost_layer searching for starting points.
     *   Create a btree of starting points ordered by increasing costs.
     */
    if (!have_start_points) {

	int dsize2;

	cum_fd = G_open_cell_old(start_layer, "");
	if (cum_fd < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  start_layer);

	data_type2 = G_get_raster_map_type(cum_fd);

	dsize2 = G_raster_size(data_type2);

	cell2 = G_allocate_raster_buf(data_type2);

	G_message(_("Reading %s... "), cum_cost_layer);
	for (row = 0; row < nrows; row++) {

	    G_percent(row, nrows, 2);
	    if (G_get_raster_row(cum_fd, cell2, row, data_type2) < 0)
		G_fatal_error(_("Unable to read raster map <%s> row %d"),
			      cum_cost_layer, row);
	    ptr2 = cell2;
	    for (col = 0; col < ncols; col++) {
		/* Did I understand that concept of cummulative cost map? - (pmx) 12 april 2000 */
		if (!G_is_null_value(ptr2, data_type2)) {
		    double cellval;

		    if (start_with_raster_vals == 1) {
			cellval = G_get_raster_value_d(ptr2, data_type2);
			new_cell = insert(cellval, row, col);
			segment_put(&out_seg, &cellval, row, col);
		    }
		    else {
			value_start_pt = &zero;
			new_cell = insert(zero, row, col);
			segment_put(&out_seg, value_start_pt, row, col);
		    }
		}
		ptr2 = G_incr_void_ptr(ptr2, dsize2);
	    }
	}

	G_percent(row, nrows, 2);

	G_close_cell(cum_fd);
	G_free(cell2);

    }


    /*  If the starting points are given on the command line start a linked
     *  list of cells ordered by increasing costs
     */
    else {
	struct start_pt *top_start_pt = NULL;

	top_start_pt = head_start_pt;
	while (top_start_pt != NULL) {
	    value_start_pt = &zero;
	    if (top_start_pt->row < 0 || top_start_pt->row >= nrows
		|| top_start_pt->col < 0 || top_start_pt->col >= ncols)
		G_fatal_error(_("Specified starting location outside database window"));
	    new_cell = insert(zero, top_start_pt->row, top_start_pt->col);
	    segment_put(&out_seg, value_start_pt, top_start_pt->row,
			top_start_pt->col);
	    top_start_pt = top_start_pt->next;
	}
    }

    /*  Loop through the btree and perform at each cell the following:
     *   1) If an adjacent cell has not already been assigned a value compute
     *      the min cost and assign it.
     *   2) Insert the adjacent cell in the btree.
     *   3) Free the memory allocated to the present cell.
     */


    /*system("date"); */
    G_message(_("Finding cost path"));

    n_processed = 0;
    total_cells = nrows * ncols;
    at_percent = 0;

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

	/* If we have surpassed the user specified maximum cost, then quit */
	if (maxcost && ((double)maxcost < pres_cell->min_cost))
	    break;

	/* If I've already been updated, delete me */
	segment_get(&out_seg, &old_min_cost, pres_cell->row, pres_cell->col);
	if (!G_is_d_null_value(&old_min_cost)) {
	    if (pres_cell->min_cost > old_min_cost) {
		delete(pres_cell);
		pres_cell = get_lowest();
		continue;
	    }
	}

	segment_get(&dtm_in_seg, &my_dtm, pres_cell->row, pres_cell->col);
	if (G_is_d_null_value(&my_dtm))
	    continue;
	segment_get(&cost_in_seg, &my_cost, pres_cell->row, pres_cell->col);
	if (G_is_d_null_value(&my_cost))
	    continue;


	G_percent(++n_processed, total_cells, 1);

	/*          9    10       Order in which neighbors 
	 *       13 5  3  6 14    are visited (Knight move).
	 *          1     2
	 *       16 8  4  7 15
	 *         12    11
	 */
	for (neighbor = 1; neighbor <= total_reviewed; neighbor++) {
	    switch (neighbor) {
	    case 1:
		row = pres_cell->row;
		col = pres_cell->col - 1;
		break;
	    case 2:
		row = pres_cell->row;
		col = pres_cell->col + 1;
		break;
	    case 3:
		row = pres_cell->row - 1;
		col = pres_cell->col;
		break;
	    case 4:
		row = pres_cell->row + 1;
		col = pres_cell->col;
		break;
	    case 5:
		row = pres_cell->row - 1;
		col = pres_cell->col - 1;
		break;
	    case 6:
		row = pres_cell->row - 1;
		col = pres_cell->col + 1;
		break;
	    case 7:
		col = pres_cell->col + 1;
		row = pres_cell->row + 1;
		break;
	    case 8:
		col = pres_cell->col - 1;
		row = pres_cell->row + 1;
		break;
	    case 9:
		row = pres_cell->row - 2;
		col = pres_cell->col - 1;
		break;
	    case 10:
		row = pres_cell->row - 2;
		col = pres_cell->col + 1;
		break;
	    case 11:
		row = pres_cell->row + 2;
		col = pres_cell->col + 1;
		break;
	    case 12:
		row = pres_cell->row + 2;
		col = pres_cell->col - 1;
		break;
	    case 13:
		row = pres_cell->row - 1;
		col = pres_cell->col - 2;
		break;
	    case 14:
		row = pres_cell->row - 1;
		col = pres_cell->col + 2;
		break;
	    case 15:
		row = pres_cell->row + 1;
		col = pres_cell->col + 2;
		break;
	    case 16:
		row = pres_cell->row + 1;
		col = pres_cell->col - 2;
		break;
	    }

	    if (row < 0 || row >= nrows)
		continue;
	    if (col < 0 || col >= ncols)
		continue;

	    switch (neighbor) {
	    case 1:
		dtm_value = &W_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &W_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((W_dtm - my_dtm) / EW_fac) >= 0)
		    fcost_dtm = (double)((double)(W_dtm - my_dtm) * b);
		else if (((W_dtm - my_dtm) / EW_fac) < (slope_factor))
		    fcost_dtm = (double)((double)(W_dtm - my_dtm) * d);
		else
		    fcost_dtm = (double)((double)(W_dtm - my_dtm) * c);
		fcost_cost = ((double)(W_cost + my_cost) / 2.0);
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (EW_fac * a) +
		    lambda * fcost_cost * EW_fac;
		break;
	    case 2:
		dtm_value = &E_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &E_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((E_dtm - my_dtm) / EW_fac) >= 0)
		    fcost_dtm = (double)(E_dtm - my_dtm) * b;
		else if (((E_dtm - my_dtm) / EW_fac) < (slope_factor))
		    fcost_dtm = (double)(E_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(E_dtm - my_dtm) * c;
		fcost_cost = (double)(E_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (EW_fac * a) +
		    lambda * fcost_cost * EW_fac;
		break;
	    case 3:
		dtm_value = &N_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &N_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((N_dtm - my_dtm) / NS_fac) >= 0)
		    fcost_dtm = (double)(N_dtm - my_dtm) * b;
		else if (((N_dtm - my_dtm) / NS_fac) < (slope_factor))
		    fcost_dtm = (double)(N_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(N_dtm - my_dtm) * c;
		fcost_cost = (double)(N_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (NS_fac * a) +
		    lambda * fcost_cost * NS_fac;
		break;
	    case 4:
		dtm_value = &S_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &S_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((S_dtm - my_dtm) / NS_fac) >= 0)
		    fcost_dtm = (double)(S_dtm - my_dtm) * b;
		else if (((S_dtm - my_dtm) / NS_fac) < (slope_factor))
		    fcost_dtm = (double)(S_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(S_dtm - my_dtm) * c;
		fcost_cost = (double)(S_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (NS_fac * a) +
		    lambda * fcost_cost * NS_fac;
		break;
	    case 5:
		dtm_value = &NW_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &NW_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((NW_dtm - my_dtm) / DIAG_fac) >= 0)
		    fcost_dtm = (double)(NW_dtm - my_dtm) * b;
		else if (((NW_dtm - my_dtm) / DIAG_fac) < (slope_factor))
		    fcost_dtm = (double)(NW_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(NW_dtm - my_dtm) * c;
		fcost_cost = (double)(NW_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (DIAG_fac * a) +
		    lambda * fcost_cost * DIAG_fac;
		break;
	    case 6:
		dtm_value = &NE_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &NE_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((NE_dtm - my_dtm) / DIAG_fac) >= 0)
		    fcost_dtm = (double)(NE_dtm - my_dtm) * b;
		else if (((NE_dtm - my_dtm) / DIAG_fac) < (slope_factor))
		    fcost_dtm = (double)(NE_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(NE_dtm - my_dtm) * c;
		fcost_cost = (double)(NE_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (DIAG_fac * a) +
		    lambda * fcost_cost * DIAG_fac;
		break;
	    case 7:
		dtm_value = &SE_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &SE_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((SE_dtm - my_dtm) / DIAG_fac) >= 0)
		    fcost_dtm = (double)(SE_dtm - my_dtm) * b;
		else if (((SE_dtm - my_dtm) / DIAG_fac) < (slope_factor))
		    fcost_dtm = (double)(SE_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(SE_dtm - my_dtm) * c;
		fcost_cost = (double)(SE_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (DIAG_fac * a) +
		    lambda * fcost_cost * DIAG_fac;
		break;
	    case 8:
		dtm_value = &SW_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &SW_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((SW_dtm - my_dtm) / DIAG_fac) >= 0)
		    fcost_dtm = (double)(SW_dtm - my_dtm) * b;
		else if (((SW_dtm - my_dtm) / DIAG_fac) < (slope_factor))
		    fcost_dtm = (double)(SW_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(SW_dtm - my_dtm) * c;
		fcost_cost = (double)(SW_cost + my_cost) / 2.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (DIAG_fac * a) +
		    lambda * fcost_cost * DIAG_fac;
		break;
	    case 9:
		dtm_value = &NNW_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &NNW_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((NNW_dtm - my_dtm) / V_DIAG_fac) >= 0)
		    fcost_dtm = (double)(NNW_dtm - my_dtm) * b;
		else if (((NNW_dtm - my_dtm) / V_DIAG_fac) < (slope_factor))
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
		dtm_value = &NNE_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &NNE_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((NNE_dtm - my_dtm) / V_DIAG_fac) >= 0)
		    fcost_dtm = (double)(NNE_dtm - my_dtm) * b;
		else if (((NNE_dtm - my_dtm) / V_DIAG_fac) < (slope_factor))
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
		dtm_value = &SSE_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &SSE_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((SSE_dtm - my_dtm) / V_DIAG_fac) >= 0)
		    fcost_dtm = (double)(SSE_dtm - my_dtm) * b;
		else if (((SSE_dtm - my_dtm) / V_DIAG_fac) < (slope_factor))
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
		dtm_value = &SSW_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &SSW_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((SSW_dtm - my_dtm) / V_DIAG_fac) >= 0)
		    fcost_dtm = (double)(SSW_dtm - my_dtm) * b;
		else if (((SSW_dtm - my_dtm) / V_DIAG_fac) < (slope_factor))
		    fcost_dtm = (double)(SSW_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(SSW_dtm - my_dtm) * c;
		fcost_cost =
		    (double)(S_cost + SW_cost + SSW_cost + my_cost) / 4.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (V_DIAG_fac * a) +
		    lambda * fcost_cost * V_DIAG_fac;
		break;
	    case 13:
		dtm_value = &WNW_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &WNW_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((WNW_dtm - my_dtm) / H_DIAG_fac) >= 0)
		    fcost_dtm = (double)(WNW_dtm - my_dtm) * b;
		else if (((WNW_dtm - my_dtm) / H_DIAG_fac) < (slope_factor))
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
		dtm_value = &ENE_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &ENE_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((ENE_dtm - my_dtm) / H_DIAG_fac) >= 0)
		    fcost_dtm = (double)(ENE_dtm - my_dtm) * b;
		else if (((ENE_dtm - my_dtm) / H_DIAG_fac) < (slope_factor))
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
		dtm_value = &ESE_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &ESE_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((ESE_dtm - my_dtm) / H_DIAG_fac) >= 0)
		    fcost_dtm = (double)(ESE_dtm - my_dtm) * b;
		else if (((ESE_dtm - my_dtm) / H_DIAG_fac) < (slope_factor))
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
		dtm_value = &WSW_dtm;
		segment_get(&dtm_in_seg, dtm_value, row, col);
		cost_value = &WSW_cost;
		segment_get(&cost_in_seg, cost_value, row, col);
		if (G_is_d_null_value(cost_value))
		    continue;
		if (((WSW_dtm - my_dtm) / H_DIAG_fac) >= 0)
		    fcost_dtm = (double)(WSW_dtm - my_dtm) * b;
		else if (((WSW_dtm - my_dtm) / H_DIAG_fac) < (slope_factor))
		    fcost_dtm = (double)(WSW_dtm - my_dtm) * d;
		else
		    fcost_dtm = (double)(WSW_dtm - my_dtm) * c;
		fcost_cost =
		    (double)(W_cost + SW_cost + WSW_cost + my_cost) / 4.0;
		min_cost =
		    pres_cell->min_cost + fcost_dtm + (H_DIAG_fac * a) +
		    lambda * fcost_cost * H_DIAG_fac;
		break;
	    }

	    if (G_is_d_null_value(&min_cost))
		continue;

	    segment_get(&out_seg, &old_min_cost, row, col);

	    if (G_is_d_null_value(&old_min_cost)) {
		segment_put(&out_seg, &min_cost, row, col);
		new_cell = insert(min_cost, row, col);
	    }
	    else {
		if (old_min_cost > min_cost) {
		    segment_put(&out_seg, &min_cost, row, col);
		    new_cell = insert(min_cost, row, col);
		}
		else {
		}
	    }
	}

	if (have_stop_points && time_to_stop(pres_cell->row, pres_cell->col))
	    break;

	ct = pres_cell;
	delete(pres_cell);

	pres_cell = get_lowest();
	if (pres_cell == NULL) {

	    G_message(_("End of map!"));
	    goto OUT;
	}
	if (ct == pres_cell)
	    G_warning(_("Error, ct == pres_cell"));
    }
  OUT:
    /*  Open cumulative cost layer for writing   */

    cum_fd = G_open_raster_new(cum_cost_layer, cum_data_type);
    cum_cell = G_allocate_raster_buf(cum_data_type);
    /*  Write pending updates by segment_put() to output map   */

    segment_flush(&out_seg);

    /*  Copy segmented map to output map  */

    /* system("date"); */
    G_message(_("Writing output raster map %s... "), cum_cost_layer);

    if (keep_nulls) {

	G_message(_("Will copy input map null values into output map"));
	cell2 = G_allocate_raster_buf(dtm_data_type);
    }
    if (cum_data_type == CELL_TYPE) {
	int *p;
	int *p2;

	G_message(_("Integer cell type.\nWriting..."));
	for (row = 0; row < nrows; row++) {

	    G_percent(row, nrows, 2);
	    if (keep_nulls) {
		if (G_get_raster_row(dtm_fd, cell2, row, dtm_data_type) < 0)
		    G_fatal_error(_("Unable to read raster map <%s> row %d"),
				  dtm_layer, row);
	    }
	    p = cum_cell;
	    p2 = cell2;
	    for (col = 0; col < ncols; col++) {
		if (keep_nulls) {
		    if (G_is_null_value(p2++, dtm_data_type)) {
			G_set_null_value((p + col), 1, dtm_data_type);
			continue;
		    }
		}
		segment_get(&out_seg, &min_cost, row, col);
		if (G_is_d_null_value(&min_cost)) {
		    G_set_null_value((p + col), 1, cum_data_type);
		}
		else {
		    if (min_cost > peak)
			peak = min_cost;
		    *(p + col) = (int)(min_cost + .5);
		}
	    }
	    G_put_raster_row(cum_fd, cum_cell, cum_data_type);
	}
    }
    else if (cum_data_type == FCELL_TYPE) {
	float *p;
	float *p2;

	G_message(_("Float cell type.\nWriting..."));
	for (row = 0; row < nrows; row++) {

	    G_percent(row, nrows, 2);
	    if (keep_nulls) {
		if (G_get_raster_row(dtm_fd, cell2, row, dtm_data_type) < 0)
		    G_fatal_error(_("Unable to read raster map <%s> row %d"),
				  dtm_layer, row);
	    }
	    p = cum_cell;
	    p2 = cell2;
	    for (col = 0; col < ncols; col++) {
		if (keep_nulls) {
		    if (G_is_null_value(p2++, dtm_data_type)) {
			G_set_null_value((p + col), 1, dtm_data_type);
			continue;
		    }
		}
		segment_get(&out_seg, &min_cost, row, col);
		if (G_is_d_null_value(&min_cost)) {
		    G_set_null_value((p + col), 1, cum_data_type);
		}
		else {
		    if (min_cost > peak)
			peak = min_cost;
		    *(p + col) = (float)(min_cost);
		}
	    }
	    G_put_raster_row(cum_fd, cum_cell, cum_data_type);
	}
    }
    else if (cum_data_type == DCELL_TYPE) {
	double *p;
	double *p2;

	G_message(_("Double cell type.\nWriting..."));
	for (row = 0; row < nrows; row++) {

	    G_percent(row, nrows, 2);
	    if (keep_nulls) {
		if (G_get_raster_row(dtm_fd, cell2, row, dtm_data_type) < 0)
		    G_fatal_error(_("Unable to read raster map <%s> row %d"),
				  cell2, row);
	    }
	    p = cum_cell;
	    p2 = cell2;
	    for (col = 0; col < ncols; col++) {
		if (keep_nulls) {
		    if (G_is_null_value(p2++, dtm_data_type)) {
			G_set_null_value((p + col), 1, dtm_data_type);
			continue;
		    }
		}
		segment_get(&out_seg, &min_cost, row, col);
		if (G_is_d_null_value(&min_cost)) {
		    G_set_null_value((p + col), 1, cum_data_type);
		}
		else {
		    if (min_cost > peak)
			peak = min_cost;
		    *(p + col) = min_cost;
		}
	    }
	    G_put_raster_row(cum_fd, cum_cell, cum_data_type);
	}
    }


    G_percent(row, nrows, 2);


    G_message(_("Peak cost value: %f"), peak);

    segment_release(&dtm_in_seg);	/* release memory  */
    segment_release(&out_seg);
    G_close_cell(dtm_fd);
    G_close_cell(cost_fd);
    G_close_cell(cum_fd);
    close(dtm_in_fd);		/* close all files */
    close(out_fd);
    close(cost_in_fd);
    unlink(dtm_in_file);	/* remove submatrix files  */
    unlink(cost_in_file);
    unlink(out_file);

    /*  Create colours for output map    */

    /*
     * G_read_range (cum_cost_layer, "", &range);
     * G_get_range_min_max(&range, &min, &max);
     * G_make_color_wave(&colors,min, max);
     * G_write_colors (cum_cost_layer,"",&colors);
     */

    /* writing history file */
    G_short_history(cum_cost_layer, "raster", &history);
    G_command_history(&history);
    G_write_history(cum_cost_layer, &history);

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
