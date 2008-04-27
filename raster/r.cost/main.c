/****************************************************************************
 *
 * MODULE:       r.cost
 *
 * AUTHOR(S):    Antony Awaida - IESL - M.I.T.
 *               James Westervelt - CERL
 *               Pierre de Mouveaux <pmx audiovu com>
 *               Eric G. Miller <egm2 jps net>
 *
 * PURPOSE:      Outputs a raster map layer showing the cumulative cost 
 *               of moving between different geographic locations on an 
 *               input raster map layer whose cell category values 
 *               represent cost.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

/*********************************************************************
 *
 *     This is the main program for the minimum path cost analysis.
 *     It generates a cumulative cost map (output) from an elevation
 *     or cost map (input) with respect to starting locations (coor).
 *
 *     It takes as input the following:
 *     1) Cost of traversing each grid cell as given by a cost map
 *        cell (input).
 *     2) If starting points are not specified on the command line
 *        then the ouput map must exist and contain the starting locations
 *
 *        Otherwise the ouput map need not exist and the coor points
 *        from the command line are used.
 *
 *********************************************************************/

/* BUG 2005: r.cost probably hangs with negative costs.
 * 
 * 08 april 2000 - Pierre de Mouveaux. pmx@audiovu.com
 * Updated to use the Grass 5.0 floating point raster cell format.
 * 
 * 12 dec 2001 - Eric G. Miller <egm2@jps.net>
 * Try to fix some file searching bugs, and give better error
 * if "output" doesn't exist, but is expected (this is bad design).
 */

#define MAIN

#define SEGCOLSIZE 	256

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

struct Cell_head window;


int main(int argc, char *argv[])
{
    void *cell, *cell2;
    SEGMENT in_seg, out_seg;
    char *cost_mapset;
    char *in_file, *out_file;
    char *search_mapset;
    double *value;
    extern struct Cell_head window;
    double NS_fac, EW_fac, DIAG_fac, H_DIAG_fac, V_DIAG_fac;
    double fcost;
    double min_cost, old_min_cost;
    double zero = 0.0;
    int at_percent = 0;
    int col, row, nrows, ncols;
    int maxcost;
    int maxmem;
    double cost;
    int cost_fd, cum_fd;
    int have_stop_points = 0;
    int in_fd, out_fd;
    double my_cost;
    double null_cost;
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
    struct Option *opt9, *opt10;
    struct cost *pres_cell, *new_cell;
    struct start_pt *pres_start_pt = NULL;
    struct start_pt *pres_stop_pt = NULL;

    void *ptr2;
    RASTER_MAP_TYPE data_type;
    struct History history;
    double peak = 0.0;
    int dsize;

    /* please, remove before GRASS 7 released */
    struct Flag *v_flag;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster, cost surface, cumulative costs");
    module->description =
	_("Outputs a raster map layer showing the "
	  "cumulative cost of moving between different "
	  "geographic locations on an input raster map "
	  "layer whose cell category values represent cost.");

    opt2 = G_define_standard_option(G_OPT_R_INPUT);
    opt2->description =
	_("Name of raster map containing grid cell cost information");

    opt1 = G_define_standard_option(G_OPT_R_OUTPUT);

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
    opt9->gisprompt = "old,cell,raster";
    opt9->required = NO;
    opt9->description = _("Starting points raster map");

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
    opt5->key_desc = "cost";
    opt5->required = NO;
    opt5->multiple = NO;
    opt5->answer = "0";
    opt5->description = _("An optional maximum cumulative cost");

    opt6 = G_define_option();
    opt6->key = "null_cost";
    opt6->type = TYPE_DOUBLE;
    opt6->key_desc = "null cost";
    opt6->required = NO;
    opt6->multiple = NO;
    opt6->description =
	_("Cost assigned to null cells. By default, null cells are excluded");

    opt10 = G_define_option();
    opt10->key = "percent_memory";
    opt10->type = TYPE_INTEGER;
    opt10->key_desc = "percent memory";
    opt10->required = NO;
    opt10->multiple = NO;
    opt10->answer = "100";
    opt10->description = _("Percent of map to keep in memory");

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

    /* please, remove before GRASS 7 released */
    v_flag = G_define_flag() ;
    v_flag->key         = 'v' ;  
    v_flag->description = _("Run verbosely") ;

    /*   Parse command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* please, remove before GRASS 7 released */
    if(v_flag->answer) {
        G_putenv("GRASS_VERBOSE","2");
        G_warning(_("The '-v' flag is superseded and will be removed "
            "in future. Please use '--verbose' instead."));
    }

    /* Initalize access to database and create temporary files */

    in_file = G_tempfile();
    out_file = G_tempfile();

    /*  Get database window parameters      */

    if (G_get_window(&window) < 0)
	G_fatal_error(_("Unable to read current window parameters"));

    /*  Find north-south, east_west and diagonal factors */

    EW_fac = 1.0;
    NS_fac = window.ns_res / window.ew_res;
    DIAG_fac = (double)sqrt((double)(NS_fac * NS_fac + EW_fac * EW_fac));
    V_DIAG_fac = (double)sqrt((double)(4 * NS_fac * NS_fac + EW_fac * EW_fac));
    H_DIAG_fac = (double)sqrt((double)(NS_fac * NS_fac + 4 * EW_fac * EW_fac));

    G_set_d_null_value(&null_cost, 1);

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
	    G_fatal_error(_
			  ("Must specify exactly one of start_points, start_rast or coordinate"));
    }

    if (opt3->answers)
	if (!process_answers(opt3->answers, &head_start_pt, &pres_start_pt))
	    G_fatal_error(_("No start points"));

    if (opt4->answers)
	have_stop_points =
	    process_answers(opt4->answers, &head_end_pt, &pres_stop_pt);

    if (sscanf(opt5->answer, "%d", &maxcost) != 1 || maxcost < 0)
	G_fatal_error(_("Inappropriate maximum cost: %d"), maxcost);

    if (sscanf(opt10->answer, "%d", &maxmem) != 1 || maxmem < 0 || maxmem > 100)
	G_fatal_error(_("Inappropriate percent memory: %d"), maxmem);

    if ((opt6->answer == NULL) ||
	(sscanf(opt6->answer, "%lf", &null_cost) != 1)) {
        G_message (_("Null cells excluded from cost evaluation."));
	G_set_d_null_value(&null_cost, 1);
    }
    else if (keep_nulls)
	G_message (_("Input null cell will be retained into output map"));

    if (opt7->answer) {
	search_mapset = G_find_vector(opt7->answer, "");
	if (search_mapset == NULL)
	    G_fatal_error(_("Vector map <%s> not found"), opt7->answer);
    }

    if (!G_is_d_null_value(&null_cost)) {
	if (null_cost < 0.0) {
	    G_warning (_("Warning: assigning negative cost to null cell. Null cells excluded."));
	    G_set_d_null_value(&null_cost, 1);
	}
    }
    else {
	keep_nulls = 0;		/* handled automagically... */
    }

    strcpy(cum_cost_layer, opt1->answer);

    /*  Check if cost layer exists in data base  */

    strcpy(cost_layer, opt2->answer);
    cost_mapset = G_find_cell2(cost_layer, "");

    if (cost_mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), cost_layer);

    /*  Check if specified output layer name is legal   */

    if (G_legal_filename(cum_cost_layer) < 0)
	G_fatal_error(_("<%s> is an illegal file name"), cum_cost_layer);

    /*  Find number of rows and columns in window    */

    nrows = G_window_rows();
    ncols = G_window_cols();


    /*  Open cost cell layer for reading  */

    cost_fd = G_open_cell_old(cost_layer, cost_mapset);

    if (cost_fd < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), cost_layer);

    data_type = G_get_raster_map_type(cost_fd);
    cell = G_allocate_raster_buf(data_type);

    /*   Parameters for map submatrices   */

    switch (data_type) {
    case (CELL_TYPE):
        G_message(_("Source map is: Integer cell type"));
        break;
    case (FCELL_TYPE):
        G_message(_("Source map is: Floating point (float) cell type"));
        break;
    case (DCELL_TYPE):
        G_message(_("Source map is: Floating point (double) cell type"));
        break;
    }
    G_message(_(" %d rows, %d cols"), nrows, ncols);

    srows = scols = SEGCOLSIZE;
    if (maxmem > 0)
	segments_in_memory =
	    2 + maxmem * (nrows / SEGCOLSIZE) * (ncols / SEGCOLSIZE) / 100;
    else
	segments_in_memory = 4 * (nrows / SEGCOLSIZE + ncols / SEGCOLSIZE + 2);

    /*   Create segmented format files for cost layer and output layer  */

    G_message(_("Creating some temporary files..."));

    in_fd = creat(in_file, 0666);
    segment_format(in_fd, nrows, ncols, srows, scols, sizeof(double));
    close(in_fd);

    out_fd = creat(out_file, 0666);
    segment_format(out_fd, nrows, ncols, srows, scols, sizeof(double));
    close(out_fd);

    /*   Open initialize and segment all files  */

    in_fd = open(in_file, 2);
    segment_init(&in_seg, in_fd, segments_in_memory);


    out_fd = open(out_file, 2);
    segment_init(&out_seg, out_fd, segments_in_memory);

    /*   Write the cost layer in the segmented file  */

    G_message(_("Reading %s"), cost_layer);

    {
	int i;
	double p;

	dsize = G_raster_size(data_type);
	p = 0.0;

	for (row = 0; row < nrows; row++) {
            G_percent(row, nrows, 2);
	    if (G_get_raster_row(cost_fd, cell, row, data_type) < 0)
		G_fatal_error(_("Unable to read raster map <%s> row %d"), cost_layer, row);

	    /* INPUT NULL VALUES: ??? */
	    ptr2 = cell;
	    switch (data_type) {
	    case CELL_TYPE:
		for (i = 0; i < ncols; i++) {
		    if (G_is_null_value(ptr2, data_type)) {
			p = null_cost;
		    }
		    else {
			p = *(int *)ptr2;
		    }
		    segment_put(&in_seg, &p, row, i);
		    ptr2 = G_incr_void_ptr(ptr2, dsize);
		}
		break;
	    case FCELL_TYPE:
		for (i = 0; i < ncols; i++) {
		    if (G_is_null_value(ptr2, data_type)) {
			p = null_cost;
		    }
		    else {
			p = *(float *)ptr2;
		    }
		    segment_put(&in_seg, &p, row, i);
		    ptr2 = G_incr_void_ptr(ptr2, dsize);
		}
		break;

	    case DCELL_TYPE:
		for (i = 0; i < ncols; i++) {
		    if (G_is_null_value(ptr2, data_type)) {
			p = null_cost;
		    }
		    else {
			p = *(double *)ptr2;
		    }
		    segment_put(&in_seg, &p, row, i);
		    ptr2 = G_incr_void_ptr(ptr2, dsize);
		}
		break;
	    }
	}
    }
    segment_flush(&in_seg);
    G_percent(row, nrows, 2);

    /* Initialize output map with NULL VALUES */

    /*   Initialize segmented output file  */
    G_message(_("Initializing output "));
    
    {
	double *fbuff;
	int i;

	fbuff = (double *)G_malloc(ncols * sizeof(double));

	if (fbuff == NULL)
	    G_fatal_error(_("Unable to allocate memory for segment fbuff == NULL"));

	G_set_d_null_value(fbuff, ncols);

	for (row = 0; row < nrows; row++) {
            G_percent(row, nrows, 2);

	    for (i = 0; i < ncols; i++) {
		segment_put(&out_seg, &fbuff[i], row, i);
	    }
	}
	segment_flush(&out_seg);
        G_percent(row, nrows, 2);
	G_free(fbuff);
    }

    /*   Scan the start_points layer searching for starting points.
     *   Create a btree of starting points ordered by increasing costs.
     */
    if (opt7->answer) {
#if 1
	FILE *fp;
	struct start_pt *new_start_pt;
	Site *site = NULL;	/* pointer to Site */
	int got_one = 0;
	int dims, strs, dbls;
	RASTER_MAP_TYPE cat;

	search_mapset = G_find_sites(opt7->answer, "");

	fp = G_fopen_sites_old(opt7->answer, search_mapset);

	if (G_site_describe( fp, &dims, &cat, &strs, &dbls))
	    G_fatal_error( "Failed to guess site file format\n");
        site = G_site_new_struct(cat, dims, strs, dbls);

	for (; (G_site_get(fp, site) != EOF);) {
	    if (!G_site_in_region(site, &window))
		continue;
	    got_one = 1;

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

	if (!got_one)
	    G_fatal_error(_("No start points"));
#endif
    }

    if (opt8->answer) {
#if 1
	FILE *fp;
	struct start_pt *new_start_pt;
	Site *site = NULL;	/* pointer to Site */
	int dims, strs, dbls;
	RASTER_MAP_TYPE cat;

	search_mapset = G_find_sites(opt8->answer, "");

	fp = G_fopen_sites_old(opt8->answer, search_mapset);

	if (G_site_describe( fp, &dims, &cat, &strs, &dbls))
	    G_fatal_error( "Failed to guess site file format\n");
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
#endif
    }

    if (opt9->answer) {
	int dsize2;
	int fd;
	RASTER_MAP_TYPE data_type2;
	int got_one = 0;

	search_mapset = G_find_file("cell", opt9->answer, "");

	fd = G_open_cell_old(opt9->answer, search_mapset);
	if (fd < 0)
	    G_fatal_error(_
			  ("can't open raster map [%s] needed for input coordinates"),
			  opt9->answer);

	data_type2 = G_get_raster_map_type(fd);

	dsize2 = G_raster_size(data_type2);

	cell2 = G_allocate_raster_buf(data_type2);

	if (!cell2)
	    G_fatal_error(_("Unable to allocate memory"));

        G_message(_("Reading %s"), opt9->answer);
	for (row = 0; row < nrows; row++) {
            G_percent(row, nrows, 2);
	    if (G_get_raster_row(fd, cell2, row, data_type2) < 0)
		G_fatal_error(_("Unable to read raster map <%s> row %d"), opt9->answer, row);
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
			value = &zero;
			new_cell = insert(zero, row, col);
			segment_put(&out_seg, value, row, col);
		    }
		    got_one = 1;
		}
		ptr2 = G_incr_void_ptr(ptr2, dsize2);
	    }
	}
        G_percent(row, nrows, 2);

	G_close_cell(fd);
	G_free(cell2);

	if (!got_one)
	    G_fatal_error(_("No start points"));
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
		G_fatal_error(_
			      ("Specified starting location outside database window"));
	    new_cell = insert(zero, top_start_pt->row, top_start_pt->col);
	    segment_put(&out_seg, value, top_start_pt->row, top_start_pt->col);
	    top_start_pt = top_start_pt->next;
	}
	/*              printf("--------+++++----------\n"); */
    }

    /*  Loop through the btree and perform at each cell the following:
     *   1) If an adjacent cell has not already been assigned a value compute
     *      the min cost and assign it.
     *   2) Insert the adjacent cell in the btree.
     *   3) Free the memory allocated to the present cell.
     */

    G_message(_("Finding cost path"));
    n_processed = 0;
    total_cells = nrows * ncols;
    at_percent = 0;

    pres_cell = get_lowest();
    while (pres_cell != NULL) {
	struct cost *ct;
	double N, NE, E, SE, S, SW, W, NW;
	double NNE, ENE, ESE, SSE, SSW, WSW, WNW, NNW;

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

	segment_get(&in_seg, &my_cost, pres_cell->row, pres_cell->col);

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
		col = pres_cell->col + 1;
		break;
	    case 3:
		row = pres_cell->row - 1;
		col = pres_cell->col;
		break;
	    case 4:
		row = pres_cell->row + 1;
		break;
	    case 5:
		row = pres_cell->row - 1;
		col = pres_cell->col - 1;
		break;
	    case 6:
		col = pres_cell->col + 1;
		break;
	    case 7:
		row = pres_cell->row + 1;
		break;
	    case 8:
		col = pres_cell->col - 1;
		break;
	    case 9:
		row = pres_cell->row - 2;
		col = pres_cell->col - 1;
		break;
	    case 10:
		col = pres_cell->col + 1;
		break;
	    case 11:
		row = pres_cell->row + 2;
		break;
	    case 12:
		col = pres_cell->col - 1;
		break;
	    case 13:
		row = pres_cell->row - 1;
		col = pres_cell->col - 2;
		break;
	    case 14:
		col = pres_cell->col + 2;
		break;
	    case 15:
		row = pres_cell->row + 1;
		break;
	    case 16:
		col = pres_cell->col - 2;
		break;
	    }

	    if (row < 0 || row >= nrows)
		continue;
	    if (col < 0 || col >= ncols)
		continue;

	    value = &cost;

	    switch (neighbor) {
	    case 1:
		value = &W;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(W + my_cost) / 2.0;
		min_cost = pres_cell->min_cost + fcost * EW_fac;
		break;
	    case 2:
		value = &E;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(E + my_cost) / 2.0;
		min_cost = pres_cell->min_cost + fcost * EW_fac;
		break;
	    case 3:
		value = &N;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(N + my_cost) / 2.0;
		min_cost = pres_cell->min_cost + fcost * NS_fac;
		break;
	    case 4:
		value = &S;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(S + my_cost) / 2.0;
		min_cost = pres_cell->min_cost + fcost * NS_fac;
		break;
	    case 5:
		value = &NW;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(NW + my_cost) / 2.0;
		min_cost = pres_cell->min_cost + fcost * DIAG_fac;
		break;
	    case 6:
		value = &NE;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(NE + my_cost) / 2.0;
		min_cost = pres_cell->min_cost + fcost * DIAG_fac;
		break;
	    case 7:
		value = &SE;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(SE + my_cost) / 2.0;
		min_cost = pres_cell->min_cost + fcost * DIAG_fac;
		break;
	    case 8:
		value = &SW;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(SW + my_cost) / 2.0;
		min_cost = pres_cell->min_cost + fcost * DIAG_fac;
		break;
	    case 9:
		value = &NNW;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(N + NW + NNW + my_cost) / 4.0;
		min_cost = pres_cell->min_cost + fcost * V_DIAG_fac;
		break;
	    case 10:
		value = &NNE;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(N + NE + NNE + my_cost) / 4.0;
		min_cost = pres_cell->min_cost + fcost * V_DIAG_fac;
		break;
	    case 11:
		value = &SSE;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(S + SE + SSE + my_cost) / 4.0;
		min_cost = pres_cell->min_cost + fcost * V_DIAG_fac;
		break;
	    case 12:
		value = &SSW;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(S + SW + SSW + my_cost) / 4.0;
		min_cost = pres_cell->min_cost + fcost * V_DIAG_fac;
		break;
	    case 13:
		value = &WNW;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(W + NW + WNW + my_cost) / 4.0;
		min_cost = pres_cell->min_cost + fcost * H_DIAG_fac;
		break;
	    case 14:
		value = &ENE;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(E + NE + ENE + my_cost) / 4.0;
		min_cost = pres_cell->min_cost + fcost * H_DIAG_fac;
		break;
	    case 15:
		value = &ESE;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(E + SE + ESE + my_cost) / 4.0;
		min_cost = pres_cell->min_cost + fcost * H_DIAG_fac;
		break;
	    case 16:
		value = &WSW;
		segment_get(&in_seg, value, row, col);
		fcost = (double)(W + SW + WSW + my_cost) / 4.0;
		min_cost = pres_cell->min_cost + fcost * H_DIAG_fac;
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

    cum_fd = G_open_raster_new(cum_cost_layer, data_type);

    /*  Write pending updates by segment_put() to output map   */

    segment_flush(&out_seg);

    /*  Copy segmented map to output map  */
    G_message(_("Writing %s"), cum_cost_layer);

    if (keep_nulls) {
        G_message(_
                ("Will copy input map null values into output map"));
	cell2 = G_allocate_raster_buf(data_type);
    }

     if (data_type == CELL_TYPE) {
	int *p;
	int *p2;
        G_message(_("Integer cell type.\nWriting..."));
	for (row = 0; row < nrows; row++) {
            G_percent(row, nrows, 2);
	    if (keep_nulls) {
		if (G_get_raster_row(cost_fd, cell2, row, data_type) < 0)
		    G_fatal_error(_("Error getting input null cells"));
	    }
	    p = cell;
	    p2 = cell2;
	    for (col = 0; col < ncols; col++) {
		if (keep_nulls) {
		    if (G_is_null_value(p2++, data_type)) {
			G_set_null_value((p + col), 1, data_type);
			continue;
		    }
		}
		segment_get(&out_seg, &min_cost, row, col);
		if (G_is_d_null_value(&min_cost)) {
		    G_set_null_value((p + col), 1, data_type);
		}
		else {
		    if (min_cost > peak)
			peak = min_cost;
		    *(p + col) = (int)(min_cost + .5);
		}
	    }
	    G_put_raster_row(cum_fd, cell, data_type);
	}
    }
    else if (data_type == FCELL_TYPE) {
	float *p;
	float *p2;
        G_message(_("Float cell type.\nWriting..."));
	for (row = 0; row < nrows; row++) {
            G_percent(row, nrows, 2);
	    if (keep_nulls) {
		if (G_get_raster_row(cost_fd, cell2, row, data_type) < 0)
		    G_fatal_error(_("Error getting input null cells"));
	    }
	    p = cell;
	    p2 = cell2;
	    for (col = 0; col < ncols; col++) {
		if (keep_nulls) {
		    if (G_is_null_value(p2++, data_type)) {
			G_set_null_value((p + col), 1, data_type);
			continue;
		    }
		}
		segment_get(&out_seg, &min_cost, row, col);
		if (G_is_d_null_value(&min_cost)) {
		    G_set_null_value((p + col), 1, data_type);
		}
		else {
		    if (min_cost > peak)
			peak = min_cost;
		    *(p + col) = (float)(min_cost);
		}
	    }
	    G_put_raster_row(cum_fd, cell, data_type);
	}
    }
    else if (data_type == DCELL_TYPE) {
	double *p;
	double *p2;
        G_message(_("Double cell type.\nWriting..."));
	for (row = 0; row < nrows; row++) {
            G_percent(row, nrows, 2);
	    if (keep_nulls) {
		if (G_get_raster_row(cost_fd, cell2, row, data_type) < 0)
		    G_fatal_error(_("Error getting input null cells"));
	    }
	    p = cell;
	    p2 = cell2;
	    for (col = 0; col < ncols; col++) {
		if (keep_nulls) {
		    if (G_is_null_value(p2++, data_type)) {
			G_set_null_value((p + col), 1, data_type);
			continue;
		    }
		}
		segment_get(&out_seg, &min_cost, row, col);
		if (G_is_d_null_value(&min_cost)) {
		    G_set_null_value((p + col), 1, data_type);
		}
		else {
		    if (min_cost > peak)
			peak = min_cost;
		    *(p + col) = min_cost;
		}
	    }
	    G_put_raster_row(cum_fd, cell, data_type);
	}
    }

    G_percent(row, nrows, 2);

    G_message(_("Peak cost value: %f"), peak);

    segment_release(&in_seg);	/* release memory  */
    segment_release(&out_seg);
    G_close_cell(cost_fd);
    G_close_cell(cum_fd);
    close(in_fd);		/* close all files */
    close(out_fd);
    unlink(in_file);		/* remove submatrix files  */
    unlink(out_file);

    G_short_history(cum_cost_layer, "raster", &history);
    G_command_history(&history);
    G_write_history(cum_cost_layer, &history);

    /*  Create colours for output map    */

    /*
     * G_read_range (cum_cost_layer, current_mapset, &range);
     * G_get_range_min_max(&range, &min, &max);
     * G_make_color_wave(&colors,min, max);
     * G_write_colors (cum_cost_layer,current_mapset,&colors);
     */
    exit(EXIT_SUCCESS);
}

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
	    G_warning(_("Warning, ignoring point outside window: %.4f,%.4f"), east, north);
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
