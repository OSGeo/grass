
/****************************************************************************
 *
 * MODULE:       r.cost
 *
 * AUTHOR(S):    Antony Awaida - IESL - M.I.T.
 *               James Westervelt - CERL
 *               Pierre de Mouveaux <pmx audiovu com>
 *               Eric G. Miller <egm2 jps net>
 *
 *               Updated for calculation errors and directional surface generation
 *                 Colin Nielsen <colin.nielsen gmail com>
 *               Use min heap instead of btree (faster, less memory)
 *                 Markus Metz
 *
 * PURPOSE:      Outputs a raster map layer showing the cumulative cost 
 *               of moving between different geographic locations on an 
 *               input raster map layer whose cell category values 
 *               represent cost.
 *
 * COPYRIGHT:    (C) 2006-2009 by the GRASS Development Team
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
 *        then the output map must exist and contain the starting locations
 *
 *        Otherwise the output map need not exist and the coor points
 *        from the command line are used.
 *
 *********************************************************************/

/* BUG 2005: r.cost probably hangs with negative costs.
 *           Positive costs could be a condition for Dijkstra search? MM
 * 
 * 08 april 2000 - Pierre de Mouveaux. pmx@audiovu.com
 * Updated to use the Grass 5.0 floating point raster cell format.
 * 
 * 12 dec 2001 - Eric G. Miller <egm2@jps.net>
 * Try to fix some file searching bugs, and give better error
 * if "output" doesn't exist, but is expected (this is bad design).
 */

/* TODO
 * use Vect_*() functions
 * use search tree for stop points
 * re-organize and clean up code for better readability
 * compartmentalize code, start with putting Dijkstra search into a separate function
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
#include <grass/site.h>
#include <grass/segment.h>
#include "cost.h"
#include "stash.h"
#include <grass/glocale.h>

#define SEGCOLSIZE 	64

struct Cell_head window;

struct variables
{
    char *alias;
    int position;
} variables[] = {
    {"output", CUM_COST_LAYER},
    {"input", COST_LAYER},
    {"coor", START_PT},
    {"outdir", MOVE_DIR_LAYER}
};

char cum_cost_layer[GNAME_MAX], move_dir_layer[GNAME_MAX];
char cost_layer[GNAME_MAX];
struct start_pt *head_start_pt = NULL;
struct start_pt *head_end_pt = NULL;


int main(int argc, char *argv[])
{
    void *cell, *cell2, *dir_cell;
    SEGMENT cost_seg, dir_seg;
    const char *cost_mapset, *move_dir_mapset;
    const char *in_file, *out_file, *dir_out_file;
    const char *search_mapset;
    double *value;
    extern struct Cell_head window;
    double NS_fac, EW_fac, DIAG_fac, H_DIAG_fac, V_DIAG_fac;
    double fcost;
    double min_cost, old_min_cost;
    double cur_dir;
    double zero = 0.0;
    int at_percent = 0;
    int col, row, nrows, ncols;
    int maxcost;
    int maxmem;
    int cost_fd, cum_fd, dir_fd;
    int have_stop_points = 0, dir = 0;
    int in_fd, dir_out_fd;
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
    struct Flag *flag2, *flag3, *flag4, *flag5;
    struct Option *opt1, *opt2, *opt3, *opt4, *opt5, *opt6, *opt7, *opt8;
    struct Option *opt9, *opt10, *opt11;
    struct cost *pres_cell, *new_cell;
    struct start_pt *pres_start_pt = NULL;
    struct start_pt *pres_stop_pt = NULL;
    struct cc {
	double cost_in, cost_out;
    } costs;

    void *ptr2;
    RASTER_MAP_TYPE data_type, dir_data_type = DCELL_TYPE;
    struct History history;
    double peak = 0.0;
    int dsize;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("cost surface"));
    G_add_keyword(_("cumulative costs"));
    module->description =
	_("Creates a raster map showing the "
	  "cumulative cost of moving between different "
	  "geographic locations on an input raster map "
	  "whose cell category values represent cost.");

    opt2 = G_define_standard_option(G_OPT_R_INPUT);
    opt2->description =
	_("Name of raster map containing grid cell cost information");

    opt1 = G_define_standard_option(G_OPT_R_OUTPUT);

    opt11 = G_define_option();
    opt11->key = "outdir";
    opt11->type = TYPE_STRING;
    opt11->required = NO;
    opt11->gisprompt = "new,cell,raster";
    opt11->description =
	_("Name of output raster map to contain movement directions");

    opt7 = G_define_standard_option(G_OPT_V_INPUT);
    opt7->key = "start_points";
    opt7->required = NO;
    opt7->description = _("Name of starting vector points map");
    opt7->guisection = _("Start");

    opt8 = G_define_standard_option(G_OPT_V_INPUT);
    opt8->key = "stop_points";
    opt8->required = NO;
    opt8->description = _("Name of stop vector points map");
    opt8->guisection = _("Stop");

    opt9 = G_define_standard_option(G_OPT_R_INPUT);
    opt9->key = "start_rast";
    opt9->required = NO;
    opt9->description = _("Name of starting raster points map");
    opt9->guisection = _("Start");

    opt3 = G_define_option();
    opt3->key = "coordinate";
    opt3->type = TYPE_STRING;
    opt3->key_desc = "x,y";
    opt3->multiple = YES;
    opt3->description =
	_("Map grid coordinates of a starting point (E,N)");
    opt3->guisection = _("Start");

    opt4 = G_define_option();
    opt4->key = "stop_coordinate";
    opt4->type = TYPE_STRING;
    opt4->key_desc = "x,y";
    opt4->multiple = YES;
    opt4->description =
	_("Map grid coordinates of a stopping point (E,N)");
    opt4->guisection = _("Stop");

    opt5 = G_define_option();
    opt5->key = "max_cost";
    opt5->type = TYPE_INTEGER;
    opt5->key_desc = "cost";
    opt5->required = NO;
    opt5->multiple = NO;
    opt5->answer = "0";
    opt5->description = _("Optional maximum cumulative cost");

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
    opt10->answer = "40";
    opt10->description = _("Percent of map to keep in memory");

    flag2 = G_define_flag();
    flag2->key = 'k';
    flag2->description =
	_("Use the 'Knight's move'; slower, but more accurate");

    flag3 = G_define_flag();
    flag3->key = 'n';
    flag3->description = _("Keep null values in output raster map");

    flag4 = G_define_flag();
    flag4->key = 'r';
    flag4->description = _("Start with values in raster map");
    flag4->guisection = _("Start");

    flag5 = G_define_flag();
    flag5->key = 'i';
    flag5->description = _("Only print info about disk space and memory requirements");

    /*   Parse command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* If no outdir is specified, set flag to skip all dir */
    if (opt11->answer != NULL)
	dir = 1;

    /* Initalize access to database and create temporary files */
    in_file = G_tempfile();
    out_file = G_tempfile();
    if (dir == 1)
	dir_out_file = G_tempfile();

    /*  Get database window parameters      */
    G_get_window(&window);

    /*  Find north-south, east_west and diagonal factors */
    EW_fac = 1.0;
    NS_fac = window.ns_res / window.ew_res;
    DIAG_fac = (double)sqrt((double)(NS_fac * NS_fac + EW_fac * EW_fac));
    V_DIAG_fac =
	(double)sqrt((double)(4 * NS_fac * NS_fac + EW_fac * EW_fac));
    H_DIAG_fac =
	(double)sqrt((double)(NS_fac * NS_fac + 4 * EW_fac * EW_fac));

    EW_fac /= 2.0;
    NS_fac /= 2.0;
    DIAG_fac /= 2.0;
    V_DIAG_fac /= 4.0;
    H_DIAG_fac /= 4.0;

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

    if ((opt6->answer == NULL) ||
	(sscanf(opt6->answer, "%lf", &null_cost) != 1)) {
	G_debug(1, "Null cells excluded from cost evaluation");
	Rast_set_d_null_value(&null_cost, 1);
    }
    else if (keep_nulls)
	G_debug(1, "Null cell will be retained into output raster map");

    if (opt7->answer) {
	search_mapset = G_find_vector2(opt7->answer, "");
	if (search_mapset == NULL)
	    G_fatal_error(_("Vector map <%s> not found"), opt7->answer);
    }

    if (!Rast_is_d_null_value(&null_cost)) {
	if (null_cost < 0.0) {
	    G_warning(_("Assigning negative cost to null cell. Null cells excluded."));
	    Rast_set_d_null_value(&null_cost, 1);
	}
    }
    else {
	keep_nulls = 0;		/* handled automagically... */
    }

    strcpy(cum_cost_layer, opt1->answer);

    /*  Check if cost layer exists in data base  */

    strcpy(cost_layer, opt2->answer);
    cost_mapset = G_find_raster2(cost_layer, "");

    if (cost_mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), cost_layer);

    if (dir == 1) {
	strcpy(move_dir_layer, opt11->answer);
	search_mapset = "";
	move_dir_mapset = G_find_raster2(move_dir_layer, search_mapset);
    }
    
    /*  Find number of rows and columns in window    */

    nrows = G_window_rows();
    ncols = G_window_cols();

    /*  Open cost cell layer for reading  */

    cost_fd = Rast_open_old(cost_layer, cost_mapset);

    if (cost_fd < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), cost_layer);

    data_type = Rast_get_map_type(cost_fd);
    cell = Rast_allocate_buf(data_type);

    /*   Parameters for map submatrices   */

    switch (data_type) {
    case (CELL_TYPE):
	G_debug(1, "Source map is: Integer cell type");
	break;
    case (FCELL_TYPE):
	G_debug(1, "Source map is: Floating point (float) cell type");
	break;
    case (DCELL_TYPE):
	G_debug(1, "Source map is: Floating point (double) cell type");
	break;
    }
    G_debug(1, "  %d rows, %d cols", nrows, ncols);

    /* this is most probably the limitation of r.cost for large datasets
     * segment size needs to be reduced to avoid unecessary disk IO
     * but it doesn't make sense to go down to 1
     * so we use 64 segment rows and cols for <= 200 million cells
     * for larger regions 32 segment rows and cols
     * maybe go down to 16 for > 500 million cells ? */
    if ((double) nrows * ncols > 200000000)
	srows = scols = SEGCOLSIZE / 2;
    else
	srows = scols = SEGCOLSIZE;

    if (maxmem == 100)
	srows = scols = 256;  /* large enough, avoid too much spill */

    if (maxmem > 0)
	segments_in_memory =
	    maxmem * (1 + nrows / srows) * (1 + ncols / scols) / 100;
    /* maxmem = 0 */
    else
	segments_in_memory = 4 * (nrows / srows + ncols / scols + 2);

    /* report disk space and memory requirements */
    G_message("--------------------------------------------");
    if (dir == 1) {
	double disk_mb, mem_mb;

	disk_mb = (double) nrows * ncols * 24. / 1048576.;
	mem_mb  = (double) srows * scols * 24. / 1048576. * segments_in_memory;
	mem_mb += nrows * ncols * 0.05 * 20. / 1048576.;    /* for Dijkstra search */
	G_message(_("Will need at least %.2f MB of disk space"), disk_mb);
	G_message(_("Will need at least %.2f MB of memory"), mem_mb);
	
    }
    else {
	double disk_mb, mem_mb;

	disk_mb = (double) nrows * ncols * 16. / 1048576.;
	mem_mb  = (double) srows * scols * 16. / 1048576. * segments_in_memory;
	mem_mb += nrows * ncols * 0.05 * 20. / 1048576.;    /* for Dijkstra search */
	G_message(_("Will need at least %.2f MB of disk space"), disk_mb);
	G_message(_("Will need at least %.2f MB of memory"), mem_mb);
    }
    G_message("--------------------------------------------");
    
    if (flag5->answer) {
	Rast_close(cost_fd);
	exit(EXIT_SUCCESS);
    }

    /* Create segmented format files for cost layer and output layer */

    G_verbose_message(_("Creating some temporary files..."));

    in_fd = creat(in_file, 0666);
    segment_format(in_fd, nrows, ncols, srows, scols, sizeof(struct cc));
    close(in_fd);

    if (dir == 1) {
	dir_out_fd = creat(dir_out_file, 0600);
	segment_format(dir_out_fd, nrows, ncols, srows, scols,
		       sizeof(double));
	close(dir_out_fd);
    }
    
    /* Open initialize and segment all files */

    in_fd = open(in_file, 2);
    segment_init(&cost_seg, in_fd, segments_in_memory);

    if (dir == 1) {
	dir_out_fd = open(dir_out_file, 2);
	segment_init(&dir_seg, dir_out_fd, segments_in_memory);
    }
    
    /*   Write the cost layer in the segmented file  */

    G_message(_("Reading raster map <%s>, initializing output..."),
	      G_fully_qualified_name(cost_layer, cost_mapset));
    {
	int i;
	double p;
	double dnullval;

	Rast_set_d_null_value(&dnullval, 1);

	dsize = Rast_cell_size(data_type);
	p = 0.0;

	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    if (Rast_get_row(cost_fd, cell, row, data_type) < 0)
		G_fatal_error(_("Unable to read raster map <%s> row %d"),
			      cost_layer, row);

	    /* INPUT NULL VALUES: ??? */
	    ptr2 = cell;
	    for (i = 0; i < ncols; i++) {
		if (Rast_is_null_value(ptr2, data_type)) {
		    p = null_cost;
		}
		else {
		    switch (data_type) {
		    case CELL_TYPE:
			p = *(CELL *)ptr2;
			break;
		    case FCELL_TYPE:
			p = *(FCELL *)ptr2;
			break;
		    case DCELL_TYPE:
			p = *(DCELL *)ptr2;
			break;
		    }
		}
		costs.cost_in = p;
		costs.cost_out = dnullval;
		segment_put(&cost_seg, &costs, row, i);
		ptr2 = G_incr_void_ptr(ptr2, dsize);
	    }
	}
	G_percent(1, 1, 1);
    }

    if (dir == 1) {
	double *fbuff;
	int i;

	G_message(_("Initializing directional output "));

	fbuff =
	    (double *)G_malloc((unsigned int)(ncols * sizeof(double)));
	Rast_set_d_null_value(fbuff, ncols);
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    for (i = 0; i < ncols; i++) {
		segment_put(&dir_seg, &fbuff[i], row, i);
	    }
	}
	G_percent(1, 1, 1);
	G_free(fbuff);
    }
    /*   Scan the start_points layer searching for starting points.
     *   Create a heap of starting points ordered by increasing costs.
     */
    init_heap();
    
    if (opt7->answer) {
	struct Map_info *fp;
	struct start_pt *new_start_pt;
	Site *site = NULL;	/* pointer to Site */
	int got_one = 0;
	int dims, strs, dbls;
	RASTER_MAP_TYPE cat;

	search_mapset = G_find_sites(opt7->answer, "");

	fp = G_fopen_sites_old(opt7->answer, search_mapset);

	if (G_site_describe(fp, &dims, &cat, &strs, &dbls))
	    G_fatal_error(_("Failed to guess site file format"));
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
    }

    if (opt8->answer) {
	struct Map_info *fp;
	struct start_pt *new_start_pt;
	Site *site = NULL;	/* pointer to Site */
	int dims, strs, dbls;
	RASTER_MAP_TYPE cat;

	search_mapset = G_find_sites(opt8->answer, "");

	fp = G_fopen_sites_old(opt8->answer, search_mapset);

	if (G_site_describe(fp, &dims, &cat, &strs, &dbls))
	    G_fatal_error(_("Failed to guess site file format\n"));
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

    if (opt9->answer) {
	int dsize2;
	int fd;
	RASTER_MAP_TYPE data_type2;
	int got_one = 0;

	search_mapset = G_find_raster(opt9->answer, "");

	if (search_mapset == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), opt9->answer);
	    
	fd = Rast_open_old(opt9->answer, search_mapset);
	if (fd < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  opt9->answer);

	data_type2 = Rast_get_map_type(fd);

	dsize2 = Rast_cell_size(data_type2);

	cell2 = Rast_allocate_buf(data_type2);

	if (!cell2)
	    G_fatal_error(_("Unable to allocate memory"));

	G_message(_("Reading raster map <%s>..."), opt9->answer);
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    if (Rast_get_row(fd, cell2, row, data_type2) < 0)
		G_fatal_error(_("Unable to read raster map <%s> row %d"),
			      opt9->answer, row);
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
	G_percent(1, 1, 2);

	Rast_close(fd);
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

    G_message(_("Finding cost path..."));
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
	segment_get(&cost_seg, &costs, pres_cell->row, pres_cell->col);
	old_min_cost = costs.cost_out;
	if (!Rast_is_d_null_value(&old_min_cost)) {
	    if (pres_cell->min_cost > old_min_cost) {
		delete(pres_cell);
		pres_cell = get_lowest();
		continue;
	    }
	}

	row = pres_cell->row;
	col = pres_cell->col;

	my_cost = costs.cost_in;

	G_percent(n_processed++, total_cells, 1);

	/*          9    10       Order in which neighbors 
	 *       13 5  3  6 14    are visited (Knight move).
	 *          1     2
	 *       16 8  4  7 15
	 *         12    11
	 */
	for (neighbor = 1; neighbor <= total_reviewed; neighbor++) {
	    switch (neighbor) {
	    case 1:
		col = pres_cell->col - 1;
		cur_dir = 180.0;
		break;
	    case 2:
		col = pres_cell->col + 1;
		cur_dir = 0.0;
		break;
	    case 3:
		row = pres_cell->row - 1;
		col = pres_cell->col;
		cur_dir = 90.0;
		break;
	    case 4:
		row = pres_cell->row + 1;
		cur_dir = 270.0;
		break;
	    case 5:
		row = pres_cell->row - 1;
		col = pres_cell->col - 1;
		cur_dir = 135.0;
		break;
	    case 6:
		col = pres_cell->col + 1;
		cur_dir = 45.0;
		break;
	    case 7:
		row = pres_cell->row + 1;
		cur_dir = 315.0;
		break;
	    case 8:
		col = pres_cell->col - 1;
		cur_dir = 225.0;
		break;
	    case 9:
		row = pres_cell->row - 2;
		col = pres_cell->col - 1;
		cur_dir = 112.5;
		break;
	    case 10:
		col = pres_cell->col + 1;
		cur_dir = 67.5;
		break;
	    case 11:
		row = pres_cell->row + 2;
		cur_dir = 292.5;
		break;
	    case 12:
		col = pres_cell->col - 1;
		cur_dir = 247.5;
		break;
	    case 13:
		row = pres_cell->row - 1;
		col = pres_cell->col - 2;
		cur_dir = 157.5;
		break;
	    case 14:
		col = pres_cell->col + 2;
		cur_dir = 22.5;
		break;
	    case 15:
		row = pres_cell->row + 1;
		cur_dir = 337.5;
		break;
	    case 16:
		col = pres_cell->col - 2;
		cur_dir = 202.5;
		break;
	    }

	    if (row < 0 || row >= nrows)
		continue;
	    if (col < 0 || col >= ncols)
		continue;

	    switch (neighbor) {
	    case 1:
		segment_get(&cost_seg, &costs, row, col);
		W = costs.cost_in;
		fcost = (double)(W + my_cost);
		min_cost = pres_cell->min_cost + fcost * EW_fac;
		break;
	    case 2:
		segment_get(&cost_seg, &costs, row, col);
		E = costs.cost_in;
		fcost = (double)(E + my_cost);
		min_cost = pres_cell->min_cost + fcost * EW_fac;
		break;
	    case 3:
		segment_get(&cost_seg, &costs, row, col);
		N = costs.cost_in;
		fcost = (double)(N + my_cost);
		min_cost = pres_cell->min_cost + fcost * NS_fac;
		break;
	    case 4:
		segment_get(&cost_seg, &costs, row, col);
		S = costs.cost_in;
		fcost = (double)(S + my_cost);
		min_cost = pres_cell->min_cost + fcost * NS_fac;
		break;
	    case 5:
		segment_get(&cost_seg, &costs, row, col);
		NW = costs.cost_in;
		fcost = (double)(NW + my_cost);
		min_cost = pres_cell->min_cost + fcost * DIAG_fac;
		break;
	    case 6:
		segment_get(&cost_seg, &costs, row, col);
		NE = costs.cost_in;
		fcost = (double)(NE + my_cost);
		min_cost = pres_cell->min_cost + fcost * DIAG_fac;
		break;
	    case 7:
		segment_get(&cost_seg, &costs, row, col);
		SE = costs.cost_in;
		fcost = (double)(SE + my_cost);
		min_cost = pres_cell->min_cost + fcost * DIAG_fac;
		break;
	    case 8:
		segment_get(&cost_seg, &costs, row, col);
		SW = costs.cost_in;
		fcost = (double)(SW + my_cost);
		min_cost = pres_cell->min_cost + fcost * DIAG_fac;
		break;
	    case 9:
		segment_get(&cost_seg, &costs, row, col);
		NNW = costs.cost_in;
		fcost = (double)(N + NW + NNW + my_cost);
		min_cost = pres_cell->min_cost + fcost * V_DIAG_fac;
		break;
	    case 10:
		segment_get(&cost_seg, &costs, row, col);
		NNE = costs.cost_in;
		fcost = (double)(N + NE + NNE + my_cost);
		min_cost = pres_cell->min_cost + fcost * V_DIAG_fac;
		break;
	    case 11:
		segment_get(&cost_seg, &costs, row, col);
		SSE = costs.cost_in;
		fcost = (double)(S + SE + SSE + my_cost);
		min_cost = pres_cell->min_cost + fcost * V_DIAG_fac;
		break;
	    case 12:
		segment_get(&cost_seg, &costs, row, col);
		SSW = costs.cost_in;
		fcost = (double)(S + SW + SSW + my_cost);
		min_cost = pres_cell->min_cost + fcost * V_DIAG_fac;
		break;
	    case 13:
		segment_get(&cost_seg, &costs, row, col);
		WNW = costs.cost_in;
		fcost = (double)(W + NW + WNW + my_cost);
		min_cost = pres_cell->min_cost + fcost * H_DIAG_fac;
		break;
	    case 14:
		segment_get(&cost_seg, &costs, row, col);
		ENE = costs.cost_in;
		fcost = (double)(E + NE + ENE + my_cost);
		min_cost = pres_cell->min_cost + fcost * H_DIAG_fac;
		break;
	    case 15:
		segment_get(&cost_seg, &costs, row, col);
		ESE = costs.cost_in;
		fcost = (double)(E + SE + ESE + my_cost);
		min_cost = pres_cell->min_cost + fcost * H_DIAG_fac;
		break;
	    case 16:
		segment_get(&cost_seg, &costs, row, col);
		WSW = costs.cost_in;
		fcost = (double)(W + SW + WSW + my_cost);
		min_cost = pres_cell->min_cost + fcost * H_DIAG_fac;
		break;
	    }

	    /* skip if costs could not be calculated */
	    if (Rast_is_d_null_value(&min_cost))
		continue;

	    old_min_cost = costs.cost_out;

	    /* add to list */
	    if (Rast_is_d_null_value(&old_min_cost)) {
		costs.cost_out = min_cost;
		segment_put(&cost_seg, &costs, row, col);
		new_cell = insert(min_cost, row, col);
		if (dir == 1) {
		    segment_put(&dir_seg, &cur_dir, row, col);
		}
	    }
	    /* update with lower costs */
	    else if (old_min_cost > min_cost) {
		costs.cost_out = min_cost;
		segment_put(&cost_seg, &costs, row, col);
		new_cell = insert(min_cost, row, col);
		if (dir == 1) {
		    segment_put(&dir_seg, &cur_dir, row, col);
		}
	    }
	}

	if (have_stop_points && time_to_stop(pres_cell->row, pres_cell->col))
	    break;

	ct = pres_cell;

	delete(pres_cell);

	pres_cell = get_lowest();

	if (ct == pres_cell) {
	    G_warning(_("Error, ct == pres_cell"));
	}
    }
    G_percent(1, 1, 1);

    /* free heap */
    free_heap();
    
    /*  Open cumulative cost layer for writing   */

    cum_fd = Rast_open_new(cum_cost_layer, data_type);

    /*  Copy segmented map to output map  */
    G_message(_("Writing raster map <%s>..."), cum_cost_layer);

    {
	void *p;
	void *p2;

	if (keep_nulls) {
	    cell2 = Rast_allocate_buf(data_type);
	}
	else
	    cell2 = NULL;

	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    if (keep_nulls) {
		if (Rast_get_row(cost_fd, cell2, row, data_type) < 0)
		    G_fatal_error(_("Unable to read raster map <%s> row %d"),
				  cost_layer, row);
	    }
	    p = cell;
	    p2 = cell2;
	    for (col = 0; col < ncols; col++) {
		if (keep_nulls) {
		    if (Rast_is_null_value(p2, data_type)) {
			Rast_set_null_value(p, 1, data_type);
			continue;
		    }
		}
		segment_get(&cost_seg, &costs, row, col);
		min_cost = costs.cost_out;
		if (Rast_is_d_null_value(&min_cost)) {
		    Rast_set_null_value(p, 1, data_type);
		}
		else {
		    if (min_cost > peak)
			peak = min_cost;
		    *(CELL *)p = (CELL)(min_cost + .5);

		    switch (data_type) {
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
		p = G_incr_void_ptr(p, dsize);
		p2 = G_incr_void_ptr(p2, dsize);
	    }
	    Rast_put_row(cum_fd, cell, data_type);
	}
	G_percent(1, 1, 1);
	if (keep_nulls)
	    G_free(cell2);
    }

    if (dir == 1) {
	double *p;

	dir_fd = Rast_open_new(move_dir_layer, dir_data_type);
	dir_cell = Rast_allocate_buf(dir_data_type);

	G_message(_("Writing movement direction file %s..."), move_dir_layer);
	for (row = 0; row < nrows; row++) {
	    p = dir_cell;
	    for (col = 0; col < ncols; col++) {
		segment_get(&dir_seg, &cur_dir, row, col);
		*(p + col) = cur_dir;
	    }
	    Rast_put_row(dir_fd, dir_cell, dir_data_type);
	    G_percent(row, nrows, 2);
	}
	G_percent(1, 1, 1);
    }

    segment_release(&cost_seg);	/* release memory  */
    if (dir == 1) {
	segment_release(&dir_seg);
    }
    Rast_close(cost_fd);
    Rast_close(cum_fd);
    if (dir == 1) {
	Rast_close(dir_fd);
    }
    close(in_fd);		/* close all files */
    if (dir == 1) {
	close(dir_out_fd);
    }
    unlink(in_file);		/* remove submatrix files  */
    if (dir == 1) {
	unlink(dir_out_file);
    }

    Rast_short_history(cum_cost_layer, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(cum_cost_layer, &history);

    if (dir == 1) {
	Rast_short_history(move_dir_layer, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(move_dir_layer, &history);
    }
    /*  Create colours for output map    */

    /*
     * Rast_read_range (cum_cost_layer, current_mapset, &range);
     * Rast_get_range_min_max(&range, &min, &max);
     * G_make_color_wave(&colors,min, max);
     * Rast_write_colors (cum_cost_layer,current_mapset,&colors);
     */

    G_done_msg(_("Peak cost value: %f."), peak);
    
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
