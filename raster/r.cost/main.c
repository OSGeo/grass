
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
 *               multiple directions with bitmask encoding
 *               avoid circular paths
 *                 Markus Metz
 *
 * PURPOSE:      Outputs a raster map layer showing the cumulative cost 
 *               of moving between different geographic locations on an 
 *               input raster map layer whose cell category values 
 *               represent cost.
 *
 * COPYRIGHT:    (C) 2006-2015 by the GRASS Development Team
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

/* BUG 2005 - Markus Metz
 * r.cost hangs with negative costs.
 * Non-negative costs are a requirement for Dijkstra search
 * 
 * 08 april 2000 - Pierre de Mouveaux. pmx@audiovu.com
 * Updated to use the Grass 5.0 floating point raster cell format.
 * 
 * 12 dec 2001 - Eric G. Miller <egm2@jps.net>
 * Try to fix some file searching bugs, and give better error
 * if "output" doesn't exist, but is expected (this is bad design).
 */

/* TODO
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
#include <grass/vector.h>
#include <grass/segment.h>
#include <grass/glocale.h>
#include "cost.h"
#include "stash.h"
#include "flag.h"

#define SEGCOLSIZE 	64

struct Cell_head window;

struct rc
{
    int r;
    int c;
};

static struct rc *stop_pnts = NULL;
static int n_stop_pnts = 0;
static int stop_pnts_alloc = 0;

int cmp_rc(struct rc *a, struct rc *b)
{
    if (a->r == b->r)
	return (a->c - b->c);

    return (a->r - b->r);
}

void add_stop_pnt(int r, int c);

int main(int argc, char *argv[])
{
    const char *cum_cost_layer, *move_dir_layer, *nearest_layer;
    const char *cost_layer;
    const char *cost_mapset, *search_mapset;
    void *cell, *cell2, *dir_cell, *nearest_cell;
    SEGMENT cost_seg, dir_seg, solve_seg;
    int have_solver;
    double *value;
    extern struct Cell_head window;
    double NS_fac, EW_fac, DIAG_fac, H_DIAG_fac, V_DIAG_fac;
    double fcost;
    double min_cost, old_min_cost;
    FCELL cur_dir;
    double zero = 0.0;
    int col, row, nrows, ncols;
    int maxcost;
    int nseg, nbytes;
    int maxmem;
    int segments_in_memory;
    int cost_fd, cum_fd, dir_fd, nearest_fd;
    int dir = 0;
    double my_cost, nearest;
    double null_cost, dnullval;
    int srows, scols;
    int total_reviewed;
    int keep_nulls = 1;
    int start_with_raster_vals = 1;
    int neighbor;
    long n_processed = 0;
    long total_cells;
    struct GModule *module;
    struct Flag *flag2, *flag3, *flag4, *flag5, *flag6;
    struct Option *opt1, *opt2, *opt3, *opt4, *opt5, *opt6, *opt7, *opt8;
    struct Option *opt9, *opt10, *opt11, *opt12, *opt_solve;
    struct cost *pres_cell;
    struct start_pt *head_start_pt = NULL;
    struct start_pt *next_start_pt;
    struct cc {
	double cost_in, cost_out, nearest;
    } costs;
    FLAG *visited;

    void *ptr2;
    RASTER_MAP_TYPE data_type,			 	/* input cost type */
                    cum_data_type = DCELL_TYPE, 	/* output cumulative cost type */
                    dir_data_type = FCELL_TYPE,		/* output direction type */
		    nearest_data_type = CELL_TYPE;	/* output nearest type */
    struct History history;
    double peak = 0.0;
    int dsize, nearest_size;
    double disk_mb, mem_mb, pq_mb;

    int dir_bin;
    DCELL mysolvedir[2], solvedir[2];

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("cost surface"));
    G_add_keyword(_("cumulative costs"));
    G_add_keyword(_("cost allocation"));
    module->description =
	_("Creates a raster map showing the "
	  "cumulative cost of moving between different "
	  "geographic locations on an input raster map "
	  "whose cell category values represent cost.");

    opt2 = G_define_standard_option(G_OPT_R_INPUT);
    opt2->description =
	_("Name of input raster map containing grid cell cost information");

    opt1 = G_define_standard_option(G_OPT_R_OUTPUT);

    opt_solve = G_define_standard_option(G_OPT_R_INPUT);
    opt_solve->key = "solver";
    opt_solve->required = NO;
    opt_solve->label =
	_("Name of input raster map solving equal costs");
    opt_solve->description =
	_("Helper variable to pick a direction if two directions have equal cumulative costs (smaller is better)");

    opt12 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt12->key = "nearest";
    opt12->required = NO;
    opt12->description =
	_("Name for output raster map with nearest start point");
    opt12->guisection = _("Optional outputs");

    opt11 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt11->key = "outdir";
    opt11->required = NO;
    opt11->description =
	_("Name for output raster map to contain movement directions");
    opt11->guisection = _("Optional outputs");

    opt7 = G_define_standard_option(G_OPT_V_INPUT);
    opt7->key = "start_points";
    opt7->required = NO;
    opt7->label = _("Name of starting vector points map");
    opt7->guisection = _("Start");

    opt8 = G_define_standard_option(G_OPT_V_INPUT);
    opt8->key = "stop_points";
    opt8->required = NO;
    opt8->label = _("Name of stopping vector points map");
    opt8->guisection = _("Stop");

    opt9 = G_define_standard_option(G_OPT_R_INPUT);
    opt9->key = "start_raster";
    opt9->required = NO;
    opt9->description = _("Name of starting raster points map");
    opt9->guisection = _("Start");

    opt3 = G_define_standard_option(G_OPT_M_COORDS);
    opt3->key = "start_coordinates";
    opt3->multiple = YES;
    opt3->description =
	_("Coordinates of starting point(s) (E,N)");
    opt3->guisection = _("Start");

    opt4 = G_define_standard_option(G_OPT_M_COORDS);
    opt4->key = "stop_coordinates";
    opt4->multiple = YES;
    opt4->description =
	_("Coordinates of stopping point(s) (E,N)");
    opt4->guisection = _("Stop");

    opt5 = G_define_option();
    opt5->key = "max_cost";
    opt5->type = TYPE_INTEGER;
    opt5->key_desc = "value";
    opt5->required = NO;
    opt5->multiple = NO;
    opt5->answer = "0";
    opt5->description = _("Maximum cumulative cost");

    opt6 = G_define_option();
    opt6->key = "null_cost";
    opt6->type = TYPE_DOUBLE;
    opt6->key_desc = "value";
    opt6->required = NO;
    opt6->multiple = NO;
    opt6->description =
	_("Cost assigned to null cells. By default, null cells are excluded");
    opt6->guisection = _("NULL cells");

    opt10 = G_define_standard_option(G_OPT_MEMORYMB);

    flag2 = G_define_flag();
    flag2->key = 'k';
    flag2->description =
	_("Use the 'Knight's move'; slower, but more accurate");

    flag3 = G_define_flag();
    flag3->key = 'n';
    flag3->description = _("Keep null values in output raster map");
    flag3->guisection = _("NULL cells");

    flag4 = G_define_flag();
    flag4->key = 'r';
    flag4->description = _("Start with values in raster map");
    flag4->guisection = _("Start");

    flag5 = G_define_flag();
    flag5->key = 'i';
    flag5->description = _("Print info about disk space and memory requirements and exit");

    flag6 = G_define_flag();
    flag6->key = 'b';
    flag6->description = _("Create bitmask encoded directions");
    flag6->guisection = _("Optional outputs");

    /* Parse options */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* If no outdir is specified, set flag to skip all dir */
    if (opt11->answer != NULL)
	dir = 1;

    /* Get database window parameters */
    Rast_get_window(&window);

    /* Find north-south, east_west and diagonal factors */
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

    dir_bin = 0;
    if (dir)
	dir_bin = flag6->answer;

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

    if (opt3->answers) {
	head_start_pt = process_start_coords(opt3->answers, head_start_pt);
	if (!head_start_pt)
	    G_fatal_error(_("No start points"));
    }

    if (opt4->answers) {
	if (!process_stop_coords(opt4->answers))
	    G_fatal_error(_("No stop points"));
    }

    if (sscanf(opt5->answer, "%d", &maxcost) != 1 || maxcost < 0)
	G_fatal_error(_("Inappropriate maximum cost: %d"), maxcost);

    if (sscanf(opt10->answer, "%d", &maxmem) != 1 || maxmem <= 0)
	G_fatal_error(_("Inappropriate amount of memory: %d"), maxmem);

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

    have_solver = 0;
    if (dir && opt_solve->answer) {
	search_mapset = G_find_raster2(opt_solve->answer, "");
	if (search_mapset == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), opt_solve->answer);
	have_solver = 1;
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

    cum_cost_layer = opt1->answer;
    cost_layer = opt2->answer;
    move_dir_layer = opt11->answer;
    nearest_layer = opt12->answer;

    /* Find number of rows and columns in window */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* Open cost cell layer for reading */
    cost_mapset = G_find_raster2(cost_layer, "");
    if (cost_mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), cost_layer);
    cost_fd = Rast_open_old(cost_layer, cost_mapset);

    data_type = Rast_get_map_type(cost_fd);

    /* Parameters for map submatrices */
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
     * segment size needs to be reduced to avoid unnecessary disk IO
     * but it doesn't make sense to go down to 1
     * so use 64 segment rows and cols for <= 200 million cells
     * for larger regions, 32 segment rows and cols
     * maybe go down to 16 for > 500 million cells ? */
    if ((double) nrows * ncols > 200000000)
	srows = scols = SEGCOLSIZE / 2;
    else
	srows = scols = SEGCOLSIZE;

    /* calculate total number of segments */
    nseg = ((nrows + srows - 1) / srows) * ((ncols + scols - 1) / scols);

    /* calculate disk space and memory requirements */
    /* (nrows + ncols) * 8. * 20.0 / 1048576. for Dijkstra search */
    pq_mb = ((double)nrows + ncols) * 8. * 20.0 / 1048576.;
    G_debug(1, "pq MB: %g", pq_mb);
    maxmem -= pq_mb;
    if (maxmem < 10)
	maxmem = 10;

    nbytes = 24;
    if (dir == TRUE)
	nbytes += 4;
    if (have_solver)
	nbytes += 16;

    disk_mb = (double) nrows * ncols * nbytes / 1048576.;
    segments_in_memory = maxmem / 
			 ((double) srows * scols * (nbytes / 1048576.));
    if (segments_in_memory < 4)
	segments_in_memory = 4;
    if (segments_in_memory > nseg)
	segments_in_memory = nseg;
    mem_mb = (double) srows * scols * (nbytes / 1048576.) * segments_in_memory;

    if (flag5->answer) {
	fprintf(stdout, _("Will need at least %.2f MB of disk space"), disk_mb);
        fprintf(stdout, "\n");
	fprintf(stdout, _("Will need at least %.2f MB of memory"), mem_mb);
        fprintf(stdout, "\n");
	fprintf(stdout, _("%d of %d segments are kept in memory"),
	                segments_in_memory, nseg);
        fprintf(stdout, "\n");
	Rast_close(cost_fd);
	exit(EXIT_SUCCESS);
    }

    G_verbose_message("--------------------------------------------");
    G_verbose_message(_("Will need at least %.2f MB of disk space"), disk_mb);
    G_verbose_message(_("Will need at least %.2f MB of memory"), mem_mb);
    G_verbose_message(_("%d of %d segments are kept in memory"),
	              segments_in_memory, nseg);
    G_verbose_message("--------------------------------------------");

    /* Create segmented format files for cost layer and output layer */
    G_verbose_message(_("Creating some temporary files..."));

    if (Segment_open(&cost_seg, G_tempfile(), nrows, ncols, srows, scols,
		     sizeof(struct cc), segments_in_memory) != 1)
	G_fatal_error(_("Can not create temporary file"));

    if (dir == 1) {
	if (Segment_open(&dir_seg, G_tempfile(), nrows, ncols, srows, scols,
		         sizeof(FCELL), segments_in_memory) != 1)
	    G_fatal_error(_("Can not create temporary file"));
    }

    if (have_solver) {
	int sfd;

	if (Segment_open(&solve_seg, G_tempfile(), nrows, ncols, srows, scols,
		         sizeof(DCELL) * 2, segments_in_memory) != 1)
	    G_fatal_error(_("Can not create temporary file"));

	sfd = Rast_open_old(opt_solve->answer, "");
	cell = Rast_allocate_buf(DCELL_TYPE);
	Rast_set_d_null_value(&solvedir[1], 1); 
	dsize = Rast_cell_size(DCELL_TYPE);
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    Rast_get_d_row(sfd, cell, row);
	    ptr2 = cell;
	    for (col = 0; col < ncols; col++) {
		solvedir[0] = *(DCELL *)ptr2;
		if (Segment_put(&solve_seg, solvedir, row, col) < 0)
		    G_fatal_error(_("Can not write to temporary file"));
		ptr2 = G_incr_void_ptr(ptr2, dsize);
	    }
	}
	Rast_close(sfd);
	G_free(cell);
    }

    /* Write the cost layer in the segmented file */
    G_message(_("Reading raster map <%s>, initializing output..."),
	      G_fully_qualified_name(cost_layer, cost_mapset));
    {
	int skip_nulls;
	double p;

	Rast_set_d_null_value(&dnullval, 1);
	costs.cost_out = dnullval;
	costs.nearest = 0;

	total_cells = nrows * ncols;

	skip_nulls = Rast_is_d_null_value(&null_cost);

	dsize = Rast_cell_size(data_type);
	cell = Rast_allocate_buf(data_type);
	p = 0.0;

	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    Rast_get_row(cost_fd, cell, row, data_type);

	    /* INPUT NULL VALUES: ??? */
	    ptr2 = cell;
	    for (col = 0; col < ncols; col++) {
		if (Rast_is_null_value(ptr2, data_type)) {
		    p = null_cost;
		    if (skip_nulls) {
			total_cells--;
		    }
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
		if (p < 0) {
		    G_warning(_("Negative cell value found at row %d, col %d. "
				"Setting negative value to null_cost value"),
			      row, col);
		    p = null_cost;
		}
		costs.cost_in = p;
		if (Segment_put(&cost_seg, &costs, row, col) < 0)
		    G_fatal_error(_("Can not write to temporary file"));
		ptr2 = G_incr_void_ptr(ptr2, dsize);
	    }
	}
	G_free(cell);
	G_percent(1, 1, 1);
    }

    if (dir == 1) {
	FCELL fnullval;

	G_message(_("Initializing directional output..."));
	Rast_set_f_null_value(&fnullval, 1);
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    for (col = 0; col < ncols; col++) {
		if (Segment_put(&dir_seg, &fnullval, row, col) < 0)
		    G_fatal_error(_("Can not write to temporary file"));
	    }
	}
	G_percent(1, 1, 1);
    }
    /*   Scan the start_points layer searching for starting points.
     *   Create a heap of starting points ordered by increasing costs.
     */
    init_heap();

    /* read vector with start points */
    if (opt7->answer) {
	struct Map_info In;
	struct line_pnts *Points;
	struct line_cats *Cats;
	struct bound_box box;
	int cat, type, npoints = 0;

	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();

	Vect_set_open_level(1); /* topology not required */

	if (1 > Vect_open_old(&In, opt7->answer, ""))
	    G_fatal_error(_("Unable to open vector map <%s>"), opt7->answer);

	G_message(_("Reading vector map <%s> with start points..."),
                  Vect_get_full_name(&In));
        
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
            npoints++;

	    col = (int)Rast_easting_to_col(Points->x[0], &window);
	    row = (int)Rast_northing_to_row(Points->y[0], &window);

	    next_start_pt =
		(struct start_pt *)(G_malloc(sizeof(struct start_pt)));

	    next_start_pt->row = row;
	    next_start_pt->col = col;
	    Vect_cat_get(Cats, 1, &cat);
	    next_start_pt->value = cat;
	    next_start_pt->next = head_start_pt;
	    head_start_pt = next_start_pt;
	}

	if (npoints < 1)
	    G_fatal_error(_("No start points found in vector map <%s>"), Vect_get_full_name(&In));
        else
            G_verbose_message(n_("%d point found", "%d points found", npoints), npoints);
        
	Vect_close(&In);
    }

    /* read vector with stop points */
    if (opt8->answer) {
	struct Map_info In;
	struct line_pnts *Points;
	struct line_cats *Cats;
	struct bound_box box;
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

	    col = (int)Rast_easting_to_col(Points->x[0], &window);
	    row = (int)Rast_northing_to_row(Points->y[0], &window);

	    add_stop_pnt(row, col);
	}

	Vect_close(&In);

	if (!stop_pnts)
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

	fd = Rast_open_old(opt9->answer, search_mapset);
	data_type2 = Rast_get_map_type(fd);
	nearest_data_type = data_type2;
	dsize2 = Rast_cell_size(data_type2);
	cell2 = Rast_allocate_buf(data_type2);
	if (!cell2)
	    G_fatal_error(_("Unable to allocate memory"));

	G_message(_("Reading raster map <%s> with start points..."), opt9->answer);
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    Rast_get_row(fd, cell2, row, data_type2);
	    ptr2 = cell2;
	    for (col = 0; col < ncols; col++) {
		/* Did I understand that concept of cumulative cost map? - (pmx) 12 april 2000 */
		if (!Rast_is_null_value(ptr2, data_type2)) {
		    double cellval;

		    if (Segment_get(&cost_seg, &costs, row, col) < 0)
			G_fatal_error(_("Can not read from temporary file"));

		    cellval = Rast_get_d_value(ptr2, data_type2);
		    if (start_with_raster_vals == 1) {
                        insert(cellval, row, col);
			costs.cost_out = cellval;
			costs.nearest = cellval;
			if (Segment_put(&cost_seg, &costs, row, col) < 0)
			    G_fatal_error(_("Can not write to temporary file"));
		    }
		    else {
			value = &zero;
			insert(zero, row, col);
			costs.cost_out = *value;
			costs.nearest = cellval;
			if (Segment_put(&cost_seg, &costs, row, col) < 0)
			    G_fatal_error(_("Can not write to temporary file"));
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
	    G_fatal_error(_("No start points"));
    }

    /*  Insert start points into min heap */
    if (head_start_pt) {

	next_start_pt = head_start_pt;
	while (next_start_pt != NULL) {
	    value = &zero;
	    if (next_start_pt->row < 0 || next_start_pt->row >= nrows
		|| next_start_pt->col < 0 || next_start_pt->col >= ncols)
		G_fatal_error(_("Specified starting location outside database window"));
	    insert(zero, next_start_pt->row, next_start_pt->col);
	    if (Segment_get(&cost_seg, &costs, next_start_pt->row,
			next_start_pt->col) < 0)
		G_fatal_error(_("Can not read from temporary file"));
	    costs.cost_out = *value;
	    costs.nearest = next_start_pt->value;

	    if (Segment_put(&cost_seg, &costs, next_start_pt->row,
			next_start_pt->col) < 0)
		G_fatal_error(_("Can not write to temporary file"));
	    next_start_pt = next_start_pt->next;
	}
    }

    if (n_stop_pnts > 1) {
	int i, j;
	
	/* prune stop points */
	j = 1;
	for (i = 1; i < n_stop_pnts; i++) {
	    if (stop_pnts[i].r != stop_pnts[j - 1].r ||
	        stop_pnts[i].c != stop_pnts[j - 1].c) {
		stop_pnts[j].r = stop_pnts[i].r;
		stop_pnts[j].c = stop_pnts[i].c;
		j++;
	    }
	}
	if (n_stop_pnts > j) {
	    G_message(_("Number of duplicate stop points: %d"), n_stop_pnts - j);
	    n_stop_pnts = j;
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
    G_message(_("Finding cost path..."));
    n_processed = 0;
    visited = flag_create(nrows, ncols);

    pres_cell = get_lowest();
    while (pres_cell != NULL) {
	struct cost *ct;
	double N, NE, E, SE, S, SW, W, NW;
	double NNE, ENE, ESE, SSE, SSW, WSW, WNW, NNW;

	N = NE = E = SE = S = SW = W = NW = dnullval;
	NNE = ENE = ESE = SSE = SSW = WSW = WNW = NNW = dnullval;

	/* If we have surpassed the user specified maximum cost, then quit */
	if (maxcost && ((double)maxcost < pres_cell->min_cost))
	    break;

	/* If I've already been updated, delete me */
	if (Segment_get(&cost_seg, &costs, pres_cell->row, pres_cell->col) < 0)
	    G_fatal_error(_("Can not read from temporary file"));
	old_min_cost = costs.cost_out;
	if (!Rast_is_d_null_value(&old_min_cost)) {
	    if (pres_cell->min_cost > old_min_cost) {
		delete(pres_cell);
		pres_cell = get_lowest();
		continue;
	    }
	}
	if (FLAG_GET(visited, pres_cell->row, pres_cell->col)) {
	    delete(pres_cell);
	    pres_cell = get_lowest();
	    continue;
	}
	FLAG_SET(visited, pres_cell->row, pres_cell->col);

	if (have_solver) {
	    if (Segment_get(&solve_seg, mysolvedir, pres_cell->row, pres_cell->col) < 0)
		G_fatal_error(_("Can not read from temporary file"));
	}

	my_cost = costs.cost_in;
	nearest = costs.nearest;

	row = pres_cell->row;
	col = pres_cell->col;

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
	 * X = current cell:
	 * 
	 *       292.5      247.5 
	 * 337.5 315   270  225    202.5
	 *       360     X  180
	 *  22.5  45    90  135    157.5
	 *        67.5      112.5
	 */

	/* drainage directions bitmask encoded CW from North
	 * drainage directions are set for each neighbor and must be 
	 * read as from neighbor to current cell
	 * 
	 * bit positions, zero-based, from neighbor to current cell
	 * 
	 *     X = neighbor                X = current cell
	 * 
	 *      15       8                   11      12 
	 *    14 6   7   0  9              10 2   3   4 13
	 *       5   X   1                    1   X   5
	 *    13 4   3   2 10               9 0   7   6 14
	 *      12      11                    8      15
	 */

	for (neighbor = 1; neighbor <= total_reviewed; neighbor++) {
	    switch (neighbor) {
	    case 1:
		row = pres_cell->row;
		col = pres_cell->col - 1;
		cur_dir = 360.0;
		if (dir_bin)
		    cur_dir = 1;
		break;
	    case 2:
		row = pres_cell->row;
		col = pres_cell->col + 1;
		cur_dir = 180.0;
		if (dir_bin)
		    cur_dir = 5;
		break;
	    case 3:
		row = pres_cell->row - 1;
		col = pres_cell->col;
		cur_dir = 270.0;
		if (dir_bin)
		    cur_dir = 3;
		break;
	    case 4:
		row = pres_cell->row + 1;
		col = pres_cell->col;
		cur_dir = 90.0;
		if (dir_bin)
		    cur_dir = 7;
		break;
	    case 5:
		row = pres_cell->row - 1;
		col = pres_cell->col - 1;
		cur_dir = 315.0;
		if (dir_bin)
		    cur_dir = 2;
		break;
	    case 6:
		row = pres_cell->row - 1;
		col = pres_cell->col + 1;
		cur_dir = 225.0;
		if (dir_bin)
		    cur_dir = 4;
		break;
	    case 7:
		row = pres_cell->row + 1;
		col = pres_cell->col + 1;
		cur_dir = 135.0;
		if (dir_bin)
		    cur_dir = 6;
		break;
	    case 8:
		row = pres_cell->row + 1;
		col = pres_cell->col - 1;
		cur_dir = 45.0;
		if (dir_bin)
		    cur_dir = 0;
		break;
	    case 9:
		row = pres_cell->row - 2;
		col = pres_cell->col - 1;
		cur_dir = 292.5;
		if (dir_bin)
		    cur_dir = 11;
		break;
	    case 10:
		row = pres_cell->row - 2;
		col = pres_cell->col + 1;
		cur_dir = 247.5;
		if (dir_bin)
		    cur_dir = 12;
		break;
	    case 11:
		row = pres_cell->row + 2;
		col = pres_cell->col + 1;
		cur_dir = 112.5;
		if (dir_bin)
		    cur_dir = 15;
		break;
	    case 12:
		row = pres_cell->row + 2;
		col = pres_cell->col - 1;
		cur_dir = 67.5;
		if (dir_bin)
		    cur_dir = 8;
		break;
	    case 13:
		row = pres_cell->row - 1;
		col = pres_cell->col - 2;
		cur_dir = 337.5;
		if (dir_bin)
		    cur_dir = 10;
		break;
	    case 14:
		row = pres_cell->row - 1;
		col = pres_cell->col + 2;
		cur_dir = 202.5;
		if (dir_bin)
		    cur_dir = 13;
		break;
	    case 15:
		row = pres_cell->row + 1;
		col = pres_cell->col + 2;
		cur_dir = 157.5;
		if (dir_bin)
		    cur_dir = 14;
		break;
	    case 16:
		row = pres_cell->row + 1;
		col = pres_cell->col - 2;
		cur_dir = 22.5;
		if (dir_bin)
		    cur_dir = 9;
		break;
	    }

	    if (row < 0 || row >= nrows)
		continue;
	    if (col < 0 || col >= ncols)
		continue;

	    /* skip already processed neighbors here ? */

	    min_cost = dnullval;
	    if (Segment_get(&cost_seg, &costs, row, col) < 0)
		G_fatal_error(_("Can not read from temporary file"));

	    switch (neighbor) {
	    case 1:
		W = costs.cost_in;
		fcost = (double)(W + my_cost);
		min_cost = pres_cell->min_cost + fcost * EW_fac;
		break;
	    case 2:
		E = costs.cost_in;
		fcost = (double)(E + my_cost);
		min_cost = pres_cell->min_cost + fcost * EW_fac;
		break;
	    case 3:
		N = costs.cost_in;
		fcost = (double)(N + my_cost);
		min_cost = pres_cell->min_cost + fcost * NS_fac;
		break;
	    case 4:
		S = costs.cost_in;
		fcost = (double)(S + my_cost);
		min_cost = pres_cell->min_cost + fcost * NS_fac;
		break;
	    case 5:
		NW = costs.cost_in;
		fcost = (double)(NW + my_cost);
		min_cost = pres_cell->min_cost + fcost * DIAG_fac;
		break;
	    case 6:
		NE = costs.cost_in;
		fcost = (double)(NE + my_cost);
		min_cost = pres_cell->min_cost + fcost * DIAG_fac;
		break;
	    case 7:
		SE = costs.cost_in;
		fcost = (double)(SE + my_cost);
		min_cost = pres_cell->min_cost + fcost * DIAG_fac;
		break;
	    case 8:
		SW = costs.cost_in;
		fcost = (double)(SW + my_cost);
		min_cost = pres_cell->min_cost + fcost * DIAG_fac;
		break;
	    case 9:
		NNW = costs.cost_in;
		fcost = (double)(N + NW + NNW + my_cost);
		min_cost = pres_cell->min_cost + fcost * V_DIAG_fac;
		break;
	    case 10:
		NNE = costs.cost_in;
		fcost = (double)(N + NE + NNE + my_cost);
		min_cost = pres_cell->min_cost + fcost * V_DIAG_fac;
		break;
	    case 11:
		SSE = costs.cost_in;
		fcost = (double)(S + SE + SSE + my_cost);
		min_cost = pres_cell->min_cost + fcost * V_DIAG_fac;
		break;
	    case 12:
		SSW = costs.cost_in;
		fcost = (double)(S + SW + SSW + my_cost);
		min_cost = pres_cell->min_cost + fcost * V_DIAG_fac;
		break;
	    case 13:
		WNW = costs.cost_in;
		fcost = (double)(W + NW + WNW + my_cost);
		min_cost = pres_cell->min_cost + fcost * H_DIAG_fac;
		break;
	    case 14:
		ENE = costs.cost_in;
		fcost = (double)(E + NE + ENE + my_cost);
		min_cost = pres_cell->min_cost + fcost * H_DIAG_fac;
		break;
	    case 15:
		ESE = costs.cost_in;
		fcost = (double)(E + SE + ESE + my_cost);
		min_cost = pres_cell->min_cost + fcost * H_DIAG_fac;
		break;
	    case 16:
		WSW = costs.cost_in;
		fcost = (double)(W + SW + WSW + my_cost);
		min_cost = pres_cell->min_cost + fcost * H_DIAG_fac;
		break;
	    }

	    /* skip if costs could not be calculated */
	    if (Rast_is_d_null_value(&min_cost))
		continue;

	    if (Segment_get(&cost_seg, &costs, row, col) < 0)
		G_fatal_error(_("Can not read from temporary file"));
	    old_min_cost = costs.cost_out;

	    /* add to list */
	    if (Rast_is_d_null_value(&old_min_cost)) {
		costs.cost_out = min_cost;
		costs.nearest = nearest;
		if (Segment_put(&cost_seg, &costs, row, col) < 0)
		    G_fatal_error(_("Can not write to temporary file"));
		insert(min_cost, row, col);
		if (dir == 1) {
		    if (dir_bin)
			cur_dir = (1 << (int)cur_dir);
		    if (Segment_put(&dir_seg, &cur_dir, row, col) < 0)
			G_fatal_error(_("Can not write to temporary file"));
		}
		if (have_solver) {
		    if (Segment_get(&solve_seg, solvedir, row, col) < 0)
			G_fatal_error(_("Can not read from temporary file"));
		    solvedir[1] = mysolvedir[0];
		    if (Segment_put(&solve_seg, solvedir, row, col) < 0)
			G_fatal_error(_("Can not write to temporary file"));
		}
	    }
	    /* update with lower costs */
	    else if (old_min_cost > min_cost) {
		costs.cost_out = min_cost;
		costs.nearest = nearest;
		if (Segment_put(&cost_seg, &costs, row, col) < 0)
		    G_fatal_error(_("Can not write to temporary file"));
		insert(min_cost, row, col);
		if (dir == 1) {
		    if (dir_bin)
			cur_dir = (1 << (int)cur_dir);
		    if (Segment_put(&dir_seg, &cur_dir, row, col) < 0)
			G_fatal_error(_("Can not write to temporary file"));
		}
		if (have_solver) {
		    if (Segment_get(&solve_seg, solvedir, row, col) < 0)
			G_fatal_error(_("Can not read from temporary file"));
		    solvedir[1] = mysolvedir[0];
		    if (Segment_put(&solve_seg, solvedir, row, col) < 0)
			G_fatal_error(_("Can not write to temporary file"));
		}
	    }
	    else if (old_min_cost == min_cost &&
	             (dir_bin || have_solver) && 
		     !(FLAG_GET(visited, row, col))) {
		FCELL old_dir;
		int dir_inv[16] = { 4, 5, 6, 7, 0, 1, 2, 3,
		                   12, 13, 14, 15, 8, 9, 10, 11 };
		int dir_fwd;
		int equal = 1;
		
		/* only update neighbors that have not yet been processed,
		 * otherwise we might get circular paths */

		if (have_solver) {
		    if (Segment_get(&solve_seg, solvedir, row, col) < 0)
			G_fatal_error(_("Can not read from temporary file"));
		    equal = (solvedir[1] == mysolvedir[0]);
		    if (solvedir[1] > mysolvedir[0]) {
			solvedir[1] = mysolvedir[0];
			if (Segment_put(&solve_seg, solvedir, row, col) < 0)
			    G_fatal_error(_("Can not write to temporary file"));

			costs.nearest = nearest;
			if (Segment_put(&cost_seg, &costs, row, col) < 0)
			    G_fatal_error(_("Can not write to temporary file"));

			if (dir == 1) {
			    if (dir_bin)
				cur_dir = (1 << (int)cur_dir);
			    if (Segment_put(&dir_seg, &cur_dir, row, col) < 0)
				G_fatal_error(_("Can not write to temporary file"));
			}
		    }
		}

		if (dir_bin && equal) {
		    /* this can create circular paths:
		     * set only if current cell does not point to neighbor
		     * does not avoid longer circular paths */
		    if (Segment_get(&dir_seg, &old_dir, pres_cell->row, pres_cell->col) < 0)
			G_fatal_error(_("Can not read from temporary file"));
		    dir_fwd = (1 << dir_inv[(int)cur_dir]);
		    if (!((int)old_dir & dir_fwd)) {
			if (Segment_get(&dir_seg, &old_dir, row, col) < 0)
			    G_fatal_error(_("Can not read from temporary file"));
			cur_dir = ((1 << (int)cur_dir) | (int)old_dir);
			if (Segment_put(&dir_seg, &cur_dir, row, col) < 0)
			    G_fatal_error(_("Can not write to temporary file"));
		    }
		}
	    }
	}

	if (stop_pnts && time_to_stop(pres_cell->row, pres_cell->col))
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
    flag_destroy(visited);

    if (have_solver) {
	Segment_close(&solve_seg);
    }

    /* Open cumulative cost layer for writing */
    cum_fd = Rast_open_new(cum_cost_layer, cum_data_type);
    cell = Rast_allocate_buf(cum_data_type);

    /* Open nearest start point layer */
    if (nearest_layer) {
	nearest_fd = Rast_open_new(nearest_layer, nearest_data_type);
	nearest_cell = Rast_allocate_buf(nearest_data_type);
    }
    else {
	nearest_fd = -1;
	nearest_cell = NULL;
    }
    nearest_size = Rast_cell_size(nearest_data_type);

    /* Copy segmented map to output map */
    G_message(_("Writing output raster map <%s>..."), cum_cost_layer);
    if (nearest_layer) {
	G_message(_("Writing raster map with nearest start point <%s>..."), nearest_layer);
    }

    cell2 = Rast_allocate_buf(data_type);
    {
	void *p;
	void *p2;
	void *p3;
	int cum_dsize = Rast_cell_size(cum_data_type);

	Rast_set_null_value(cell2, ncols, data_type);

	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    if (keep_nulls)
		Rast_get_row(cost_fd, cell2, row, data_type);

	    p = cell;
	    p2 = cell2;
	    p3 = nearest_cell;
	    for (col = 0; col < ncols; col++) {
		if (keep_nulls) {
		    if (Rast_is_null_value(p2, data_type)) {
			Rast_set_null_value(p, 1, cum_data_type);
			p = G_incr_void_ptr(p, cum_dsize);
			p2 = G_incr_void_ptr(p2, dsize);
			if (nearest_layer) {
			    Rast_set_null_value(p3, 1, nearest_data_type);
			    p3 = G_incr_void_ptr(p3, nearest_size);
			}

			continue;
		    }
		}
		if (Segment_get(&cost_seg, &costs, row, col) < 0)
		    G_fatal_error(_("Can not read from temporary file"));
		min_cost = costs.cost_out;
		nearest = costs.nearest;
		if (Rast_is_d_null_value(&min_cost)) {
		    Rast_set_null_value(p, 1, cum_data_type);
		    if (nearest_layer)
			Rast_set_null_value(p3, 1, nearest_data_type);
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

		    if (nearest_layer) {
			switch (nearest_data_type) {
			case CELL_TYPE:
			    *(CELL *)p3 = (CELL)(nearest);
			    break;
			case FCELL_TYPE:
			    *(FCELL *)p3 = (FCELL)(nearest);
			    break;
			case DCELL_TYPE:
			    *(DCELL *)p3 = (DCELL)(nearest);
			    break;
			}
		    }
		}
		p = G_incr_void_ptr(p, cum_dsize);
		p2 = G_incr_void_ptr(p2, dsize);
		if (nearest_layer)
		    p3 = G_incr_void_ptr(p3, nearest_size);
	    }
	    Rast_put_row(cum_fd, cell, cum_data_type);
	    if (nearest_layer)
		Rast_put_row(nearest_fd, nearest_cell, nearest_data_type);
	}
	G_percent(1, 1, 1);
	G_free(cell);
	G_free(cell2);
	if (nearest_layer)
	    G_free(nearest_cell);
    }

    if (dir == 1) {
	void *p;
	size_t dir_size = Rast_cell_size(dir_data_type);

	dir_fd = Rast_open_new(move_dir_layer, dir_data_type);
	dir_cell = Rast_allocate_buf(dir_data_type);

	G_message(_("Writing output movement direction raster map <%s>..."), move_dir_layer);
	for (row = 0; row < nrows; row++) {
	    p = dir_cell;
	    for (col = 0; col < ncols; col++) {
		if (Segment_get(&dir_seg, &cur_dir, row, col) < 0)
		    G_fatal_error(_("Can not read from temporary file"));
		*((FCELL *) p) = cur_dir;
		p = G_incr_void_ptr(p, dir_size);
	    }
	    Rast_put_row(dir_fd, dir_cell, dir_data_type);
	    G_percent(row, nrows, 2);
	}
	G_percent(1, 1, 1);
	G_free(dir_cell);
    }

    Segment_close(&cost_seg);	/* release memory  */
    if (dir == 1)
	Segment_close(&dir_seg);

    Rast_close(cost_fd);
    Rast_close(cum_fd);
    if (dir == 1)
	Rast_close(dir_fd);
    if (nearest_layer)
	Rast_close(nearest_fd);

    /* writing history file */
    Rast_short_history(cum_cost_layer, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(cum_cost_layer, &history);

    if (dir == 1) {
	Rast_short_history(move_dir_layer, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(move_dir_layer, &history);
    }

    if (nearest_layer) {
	Rast_short_history(nearest_layer, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(nearest_layer, &history);
	if (opt9->answer) {
	    struct Colors colors;
	    Rast_read_colors(opt9->answer, "", &colors);
	    Rast_write_colors(nearest_layer, G_mapset(), &colors);
	}
	else {
	    struct Colors colors;
	    struct Range range;
	    CELL min, max;
	    
	    Rast_read_range(nearest_layer, G_mapset(), &range);
	    Rast_get_range_min_max(&range, &min, &max);
	    Rast_make_random_colors(&colors, min, max);
	    Rast_write_colors(nearest_layer, G_mapset(), &colors);
	}
    }

    /* Create colours for output map */

    /*
     * Rast_read_range (cum_cost_layer, current_mapset, &range);
     * Rast_get_range_min_max(&range, &min, &max);
     * G_make_color_wave(&colors,min, max);
     * Rast_write_colors (cum_cost_layer,current_mapset,&colors);
     */

    G_done_msg(_("Peak cost value: %g"), peak);

    exit(EXIT_SUCCESS);
}

struct start_pt *
process_start_coords(char **answers, struct start_pt *top_start_pt)
{
    int col, row;
    double east, north;
    struct start_pt *new_start_pt;
    int point_no = 0;

    if (!answers)
	return (0);

    for (; *answers != NULL; answers += 2) {
	if (!G_scan_easting(*answers, &east, G_projection()))
	    G_fatal_error(_("Illegal x coordinate <%s>"), *answers);
	if (!G_scan_northing(*(answers + 1), &north, G_projection()))
	    G_fatal_error(_("Illegal y coordinate <%s>"), *(answers + 1));

	if (east < window.west || east > window.east ||
	    north < window.south || north > window.north) {
	    G_warning(_("Warning, ignoring point outside window: %g, %g"),
		      east, north);
	    continue;
	}

	row = (window.north - north) / window.ns_res;
	col = (east - window.west) / window.ew_res;

	new_start_pt = (struct start_pt *)(G_malloc(sizeof(struct start_pt)));

	new_start_pt->row = row;
	new_start_pt->col = col;
	new_start_pt->value = ++point_no;
	new_start_pt->next = top_start_pt;
	top_start_pt = new_start_pt;
    }

    return top_start_pt;
}

int process_stop_coords(char **answers)
{
    int col, row;
    double east, north;

    if (!answers)
	return 0;

    for (; *answers != NULL; answers += 2) {
	if (!G_scan_easting(*answers, &east, G_projection()))
	    G_fatal_error(_("Illegal x coordinate <%s>"), *answers);
	if (!G_scan_northing(*(answers + 1), &north, G_projection()))
	    G_fatal_error(_("Illegal y coordinate <%s>"), *(answers + 1));

	if (east < window.west || east > window.east ||
	    north < window.south || north > window.north) {
	    G_warning(_("Warning, ignoring point outside window: %g, %g"),
		      east, north);
	    continue;
	}

	row = (window.north - north) / window.ns_res;
	col = (east - window.west) / window.ew_res;

	add_stop_pnt(row, col);
    }

    return (stop_pnts != NULL);
}

void add_stop_pnt(int r, int c)
{
    int i;
    struct rc sp;

    if (n_stop_pnts == stop_pnts_alloc) {
	stop_pnts_alloc += 100;
	stop_pnts = (struct rc *)G_realloc(stop_pnts, stop_pnts_alloc * sizeof(struct rc));
    }

    sp.r = r;
    sp.c = c;
    i = n_stop_pnts;
    while (i > 0 && cmp_rc(stop_pnts + i - 1, &sp) > 0) {
	stop_pnts[i] = stop_pnts[i - 1];
	i--;
    }
    stop_pnts[i] = sp;

    n_stop_pnts++;
}

int time_to_stop(int row, int col)
{
    int lo, mid, hi;
    struct rc sp;
    static int hits = 0;

    sp.r = row;
    sp.c = col;

    lo = 0;
    hi = n_stop_pnts - 1;

    /* bsearch with deferred test for equality
     * slightly more efficient for worst case: no match */
    while (lo < hi) {
	mid = lo + ((hi - lo) >> 1);
	if (cmp_rc(stop_pnts + mid, &sp) < 0)
	    lo = mid + 1;
	else
	    hi = mid;
    }
    if (cmp_rc(stop_pnts + lo, &sp) == 0) {
	return (++hits == n_stop_pnts);
    }

    return 0;
}
