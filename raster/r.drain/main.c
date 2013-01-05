/****************************************************************************
 *
 * MODULE:       r.drain
 *               
 * AUTHOR(S):    Kewan Q. Khawaja <kewan techlogix.com>
 *               Update to FP (2000): Pierre de Mouveaux <pmx@audiovu.com> <pmx@free.fr>
 *               bugfix in FCELL, DCELL: Markus Neteler 12/2000
 *               Rewritten by Roger Miller 7/2001 based on subroutines from
 *               r.fill.dir and on the original r.drain.
 *               24 July 2004: WebValley 2004, error checking and vector points added by
 *               Matteo Franchi          Liceo Leonardo Da Vinci Trento
 *               Roberto Flor            ITC-irst
 *               New code added by Colin Nielsen <colin.nielsen at gmail dot com> *
 *               to use movement direction surface from r.walk and r.cost, and to 
 *               output vector paths 2/2009
 *               
 * PURPOSE:      This is the main program for tracing out the path that a
 *               drop of water would take if released at a certain location
 *               on an input elevation map.  
 * COPYRIGHT:    (C) 2000,2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <grass/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* for using the "open" statement */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* for using the close statement */
#include <unistd.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/vector.h>

#define DEBUG
#include "tinf.h"
#include "local.h"

/* should probably be updated to a pointer array & malloc/realloc as needed */
#define MAX_POINTS 1024

/* define a data structure to hold the point data */
struct point
{
    int row;
    int col;
    struct point *next;
    double value;
};

int main(int argc, char **argv)
{

    int fe, fd, dir_fd;
    int i, have_points = 0;
    int new_id;
    int nrows, ncols, points_row[MAX_POINTS], points_col[MAX_POINTS], npoints;
    int cell_open(), cell_open_new();
    int map_id, dir_id;
    char map_name[GNAME_MAX], new_map_name[GNAME_MAX], dir_name[GNAME_MAX];
    char *tempfile1, *tempfile2, *tempfile3;
    struct History history;

    struct Cell_head window;
    struct Option *opt1, *opt2, *coordopt, *vpointopt, *opt3, *opt4;
    struct Flag *flag1, *flag2, *flag3, *flag4;
    struct GModule *module;
    int in_type;
    void *in_buf;
    void *dir_buf;
    CELL *out_buf;
    struct band3 bnd, bndC;
    struct metrics *m = NULL;

    struct point *list;
    struct point *thispoint;
    int ival, start_row, start_col, mode;
    off_t bsz;
    int costmode = 0;
    double east, north, val;
    struct point *drain(int, struct point *, int, int);
    struct point *drain_cost(int, struct point *, int, int);
    int bsort(int, struct point *);

    struct line_pnts *Points;
    struct line_cats *Cats;
    struct Map_info vout;
    int cat;
    double x, y;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    module->description =
	_("Traces a flow through an elevation model on a raster map.");

    opt1 = G_define_standard_option(G_OPT_R_ELEV);
    opt1->key = "input";
    
    opt3 = G_define_standard_option(G_OPT_R_INPUT);
    opt3->key = "indir";
    opt3->description =
	_("Name of input movement direction map associated with the cost surface");
    opt3->required = NO;
    
    opt2 = G_define_standard_option(G_OPT_R_OUTPUT);
    
    opt4 = G_define_standard_option(G_OPT_V_OUTPUT);
    opt4->key = "vector_output";
    opt4->required = NO;
    opt4->description =
	_("Name for output drain vector map (recommended for cost surface made using knight's move)");
    
    coordopt = G_define_standard_option(G_OPT_M_COORDS);
    coordopt->key = "start_coordinates";
    coordopt->description = _("Coordinates of starting point(s) (E,N)");
    coordopt->guisection = _("Start");

    vpointopt = G_define_standard_option(G_OPT_V_INPUTS);
    vpointopt->key = "start_points";
    vpointopt->required = NO;
    vpointopt->label = _("Name of starting vector points map(s)");
    vpointopt->guisection = _("Start");

    flag1 = G_define_flag();
    flag1->key = 'c';
    flag1->description = _("Copy input cell values on output");

    flag2 = G_define_flag();
    flag2->key = 'a';
    flag2->description = _("Accumulate input values along the path");

    flag3 = G_define_flag();
    flag3->key = 'n';
    flag3->description = _("Count cell numbers along the path");

    flag4 = G_define_flag();
    flag4->key = 'd';
    flag4->description =
	_("The input surface is a cost surface (if checked, a direction surface must also be specified");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    strcpy(map_name, opt1->answer);
    strcpy(new_map_name, opt2->answer);

    if (flag4->answer) {
	costmode = 1;
	G_verbose_message(_("Directional drain selected... checking for direction raster"));
    }
    else {
	G_verbose_message(_("Surface/Hydrology drain selected"));
    }

    if (costmode == 1) {
	if (!opt3->answer) {
	    G_fatal_error(_("Direction raster not specified, if direction flag is on, "
                            "a direction raster must be given"));
	}
	strcpy(dir_name, opt3->answer);
    }
    if (costmode == 0) {
	if (opt3->answer) {
	    G_fatal_error(_("Direction map <%s> should not be specified for Surface/Hydrology drains"),
			  opt3->answer);
	}
    }

    if (opt4->answer) {
	if (0 > Vect_open_new(&vout, opt4->answer, 0)) {
            G_fatal_error(_("Unable to create vector map <%s>"),
			  opt4->answer);
	}
	Vect_hist_command(&vout);
    }
    /*      allocate cell buf for the map layer */
    in_type = Rast_map_type(map_name, "");

    /* set the pointers for multi-typed functions */
    set_func_pointers(in_type);

    if ((flag1->answer + flag2->answer + flag3->answer) > 1)
	G_fatal_error(_("Specify just one of the -c, -a and -n flags"));

    mode = 0;
    if (flag1->answer)
	mode = 1;
    if (flag2->answer)
	mode = 2;
    if (flag3->answer)
	mode = 3;

    /* get the window information  */
    G_get_window(&window);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* calculate true cell resolution */
    m = (struct metrics *)G_malloc(nrows * sizeof(struct metrics));

    if (m == NULL)
	G_fatal_error(_("Metrics allocation"));
    npoints = 0;
    if (coordopt->answer) {
	for (i = 0; coordopt->answers[i] != NULL; i += 2) {
	    G_scan_easting(coordopt->answers[i], &east, G_projection());
	    G_scan_northing(coordopt->answers[i + 1], &north, G_projection());
	    start_col = (int)Rast_easting_to_col(east, &window);
	    start_row = (int)Rast_northing_to_row(north, &window);

	    if (start_row < 0 || start_row > nrows ||
		start_col < 0 || start_col > ncols) {
		G_warning(_("Starting point %d is outside the current region"),
			  i + 1);
		continue;
	    }
	    points_row[npoints] = start_row;
	    points_col[npoints] = start_col;
	    npoints++;
	    if (npoints >= MAX_POINTS)
		G_fatal_error(_("Too many start points"));
	    have_points = 1;
	}
    }
    if (vpointopt->answers) {
	for (i = 0; vpointopt->answers[i] != NULL; i++) {
	    struct Map_info In;
	    struct bound_box box;
	    int type;

	    G_message(_("Reading vector map <%s> with start points..."), vpointopt->answers[i]);

	    Points = Vect_new_line_struct();
	    Cats = Vect_new_cats_struct();

	    Vect_set_open_level(1); /* topology not required */

	    if (1 > Vect_open_old(&In, vpointopt->answers[i], ""))
		G_fatal_error(_("Unable to open vector map <%s>"), vpointopt->answers[i]);

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

		start_col = (int)Rast_easting_to_col(Points->x[0], &window);
		start_row = (int)Rast_northing_to_row(Points->y[0], &window);

		/* effectively just a duplicate check to G_site_in_region() ??? */
		if (start_row < 0 || start_row > nrows || start_col < 0 ||
		    start_col > ncols)
		    continue;

		points_row[npoints] = start_row;
		points_col[npoints] = start_col;
		npoints++;
		if (npoints >= MAX_POINTS)
		    G_fatal_error(_("Too many start points"));
		have_points = 1;
	    }
	    Vect_close(&In);

	    /* only catches maps out of range until something is found, not after */
	    if (!have_points) {
		G_warning(_("Starting vector map <%s> contains no points in the current region"),
			  vpointopt->answers[i]);
	    }
	    Vect_destroy_line_struct(Points);
	    Vect_destroy_cats_struct(Cats);
	}
    }
    if (have_points == 0)
	G_fatal_error(_("No start/stop point(s) specified"));

    /* determine the drainage paths */

    /* allocate storage for the first point */
    thispoint = (struct point *)G_malloc(sizeof(struct point));
    list = thispoint;
    thispoint->next = NULL;

    G_begin_distance_calculations();
    {
	double e1, n1, e2, n2;

	e1 = window.east;
	n1 = window.north;
	e2 = e1 + window.ew_res;
	n2 = n1 - window.ns_res;
	for (i = 0; i < nrows; i++) {
	    m[i].ew_res = G_distance(e1, n1, e2, n1);
	    m[i].ns_res = G_distance(e1, n1, e1, n2);
	    m[i].diag_res = G_distance(e1, n1, e2, n2);
	    e2 = e1 + window.ew_res;
	    n2 = n1 - window.ns_res;
	}
    }

    /* buffers for internal use */
    bndC.ns = ncols;
    bndC.sz = sizeof(CELL) * ncols;
    bndC.b[0] = G_calloc(ncols, sizeof(CELL));
    bndC.b[1] = G_calloc(ncols, sizeof(CELL));
    bndC.b[2] = G_calloc(ncols, sizeof(CELL));

    /* buffers for external use */
    bnd.ns = ncols;
    bnd.sz = ncols * bpe();
    bnd.b[0] = G_calloc(ncols, bpe());
    bnd.b[1] = G_calloc(ncols, bpe());
    bnd.b[2] = G_calloc(ncols, bpe());

    /* an input buffer */
    in_buf = get_buf();

    /* open the original map and get its file id  */
    map_id = Rast_open_old(map_name, "");

    /* get some temp files */
    tempfile1 = G_tempfile();
    tempfile2 = G_tempfile();

    fe = open(tempfile1, O_RDWR | O_CREAT, 0666);
    fd = open(tempfile2, O_RDWR | O_CREAT, 0666);

    /* transfer the input map to a temp file */
    for (i = 0; i < nrows; i++) {
	get_row(map_id, in_buf, i);
	write(fe, in_buf, bnd.sz);
    }
    Rast_close(map_id);

    if (costmode == 1) {
	dir_buf = Rast_allocate_d_buf();
	dir_id = Rast_open_old(dir_name, "");
	tempfile3 = G_tempfile();
	dir_fd = open(tempfile3, O_RDWR | O_CREAT, 0666);

	for (i = 0; i < nrows; i++) {
	    Rast_get_d_row(dir_id, dir_buf, i);
	    write(dir_fd, dir_buf, ncols * sizeof(DCELL));
	}
	Rast_close(dir_id);
    }

    /* only necessary for non-dir drain */
    if (costmode == 0) {
	G_message(_("Calculating flow directions..."));

	/* fill one-cell pits and take a first stab at flow directions */
	filldir(fe, fd, nrows, &bnd, m);

	/* determine flow directions for more ambiguous cases */
	resolve(fd, nrows, &bndC);
    }

    /* free the buffers already used */
    G_free(bndC.b[0]);
    G_free(bndC.b[1]);
    G_free(bndC.b[2]);

    G_free(bnd.b[0]);
    G_free(bnd.b[1]);
    G_free(bnd.b[2]);

    /* determine the drainage paths */

    /* repeat for each starting point */
    for (i = 0; i < npoints; i++) {
	/* use the flow directions to determine the drainage path
	 * results are compiled as a linked list of points in downstream order */
	thispoint->row = points_row[i];
	thispoint->col = points_col[i];
	thispoint->next = NULL;
	/* drain algorithm selection (dir or non-dir) */
	if (costmode == 0) {
	    thispoint = drain(fd, thispoint, nrows, ncols);
	}
	if (costmode == 1) {
	    thispoint = drain_cost(dir_fd, thispoint, nrows, ncols);
	}
    }

    /* do the output */

    if (mode == 0 || mode == 3) {

	/* Output will be a cell map */
	/* open a new file and allocate an output buffer */
	new_id = Rast_open_c_new(new_map_name);
	out_buf = Rast_allocate_c_buf();

	/* mark each cell */
	thispoint = list;
	while (thispoint->next != NULL) {
	    thispoint->value = 1;
	    thispoint = thispoint->next;
	}

	if (mode == 3) {
	    /* number each cell downstream */
	    thispoint = list;
	    ival = 0;
	    while (thispoint->next != NULL) {
		if (thispoint->row == INT_MAX) {
		    ival = 0;
		    thispoint = thispoint->next;
		    continue;
		}
		thispoint->value += ival;
		ival = thispoint->value;
		thispoint = thispoint->next;
	    }
	}

	/* build the output map */
	G_message(_("Writing output raster map..."));
	for (i = 0; i < nrows; i++) {
	    G_percent(i, nrows, 2);
	    Rast_set_c_null_value(out_buf, ncols);
	    thispoint = list;
	    while (thispoint->next != NULL) {
		if (thispoint->row == i)
		    out_buf[thispoint->col] = (int)thispoint->value;
		thispoint = thispoint->next;
	    }
	    Rast_put_c_row(new_id, out_buf);
	}
	G_percent(1, 1, 1);
    }
    else {			/* mode = 1 or 2 */
	/* Output will be of the same type as input */
	/* open a new file and allocate an output buffer */
	new_id = Rast_open_new(new_map_name, in_type);
	out_buf = get_buf();
	bsz = ncols * bpe();

	/* loop through each point in the list and store the map values */
	thispoint = list;
	while (thispoint->next != NULL) {
	    if (thispoint->row == INT_MAX) {
		thispoint = thispoint->next;
		continue;
	    }
	    lseek(fe, (off_t) thispoint->row * bsz, SEEK_SET);
	    read(fe, in_buf, bsz);
	    memcpy(&thispoint->value, (char *)in_buf + bpe() * thispoint->col,
		   bpe());
	    thispoint = thispoint->next;
	}

	if (mode == 2) {
	    /* accumulate the input map values downstream */
	    thispoint = list;
	    val = 0.;
	    while (thispoint->next != NULL) {
		if (thispoint->row == INT_MAX) {
		    val = 0.;
		    thispoint = thispoint->next;
		    continue;
		}
		sum(&thispoint->value, &val);
		memcpy(&val, &thispoint->value, bpe());
		thispoint = thispoint->next;
	    }
	}

	/* build the output map */
	G_message(_("Writing raster map <%s>..."),
		  new_map_name);
	for (i = 0; i < nrows; i++) {
	    G_percent(i, nrows, 2);
	    set_null_value(out_buf, ncols);
	    thispoint = list;
	    while (thispoint->next != NULL) {
		if (thispoint->row == i)
		    memcpy((char *)out_buf + bpe() * thispoint->col,
			   &(thispoint->value), bpe());
		thispoint = thispoint->next;
	    }
	    put_row(new_id, out_buf);
	}
	G_percent(1, 1, 1);
    }

    /* Output a vector path */
    if (opt4->answer) {
	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();
	/* Need to modify for multiple paths */
	thispoint = list;
	i = 1;
	while (thispoint->next != NULL) {
	    if (thispoint->row == INT_MAX) {
		thispoint = thispoint->next;
		Vect_cat_set(Cats, 1, i);
		Vect_write_line(&vout, GV_LINE, Points, Cats);
		Vect_reset_line(Points);
		Vect_reset_cats(Cats);
		i++;
		continue;
	    }
	    if (Vect_cat_get(Cats, 1, &cat) == 0) {
		Vect_cat_set(Cats, 1, i);
	    }
	    /* Need to convert row and col to coordinates 
	     *      y = cell_head.north - ((double) p->row + 0.5) * cell_head.ns_res;
	     *  x = cell_head.west + ((double) p->col + 0.5) * cell_head.ew_res;
	     */

	    x = window.west + ((double)thispoint->col + 0.5) * window.ew_res;
	    y = window.north - ((double)thispoint->row + 0.5) * window.ns_res;
	    Vect_append_point(Points, x, y, 0.0);
	    thispoint = thispoint->next;
	}			/* End while */
	Vect_build(&vout);
	Vect_close(&vout);
    }

    /* close files and free buffers */
    Rast_close(new_id);

    Rast_put_cell_title(new_map_name, "Surface flow trace");

    Rast_short_history(new_map_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(new_map_name, &history);

    close(fe);
    close(fd);

    unlink(tempfile1);
    unlink(tempfile2);
    G_free(in_buf);
    G_free(out_buf);

    if (costmode == 1) {
	close(dir_fd);
	unlink(tempfile3);
	G_free(dir_buf);
    }
    
    exit(EXIT_SUCCESS);
}

struct point *drain(int fd, struct point *list, int nrow, int ncol)
{
    int go = 1, next_row, next_col;
    CELL direction;
    CELL *dir;

    dir = Rast_allocate_c_buf();
    next_row = list->row;
    next_col = list->col;

    /* begin loop */
    while (go) {

	/* find flow direction at this point */
	lseek(fd, (off_t) list->row * ncol * sizeof(CELL), SEEK_SET);
	read(fd, dir, ncol * sizeof(CELL));
	direction = *(dir + list->col);
	go = 0;

	/* identify next downstream cell */
	if (direction > 0 && direction < 256) {

	    if (direction == 1 || direction == 2 || direction == 4)
		next_col += 1;
	    else if (direction == 16 || direction == 32 || direction == 64)
		next_col -= 1;

	    if (direction == 64 || direction == 128 || direction == 1)
		next_row -= 1;
	    else if (direction == 4 || direction == 8 || direction == 16)
		next_row += 1;

	    if (next_col >= 0 && next_col < ncol
		&& next_row >= 0 && next_row < nrow) {
		/* allocate and fill the next point structure */
		list->next = (struct point *)G_malloc(sizeof(struct point));
		list = list->next;
		list->row = next_row;
		list->col = next_col;
		go = 1;
	    }
	}
    }				/* end while */

    /* allocate and fill the end-of-path flag */
    list->next = (struct point *)G_malloc(sizeof(struct point));
    list = list->next;
    list->row = INT_MAX;

    /* return a pointer to an empty structure */
    list->next = (struct point *)G_malloc(sizeof(struct point));
    list = list->next;
    list->next = NULL;

    G_free(dir);

    return list;
}

struct point *drain_cost(int dir_fd, struct point *list, int nrow, int ncol)
{
    /*
     * The idea is that each cell of the direction surface has a value representing
     * the direction towards the next cell in the path. The direction is read from 
     * the input raster, and a simple case/switch is used to determine which cell to
     * read next. This is repeated via a while loop until a null direction is found.
     */

    int neighbour, next_row, next_col, go = 1;
    DCELL direction;
    DCELL *dir_buf;

    dir_buf = Rast_allocate_d_buf();

    next_row = list->row;
    next_col = list->col;

    while (go) {
	go = 0;
	/* Directional algorithm
	 * 1) read cell direction               
	 * 2) shift to cell in that direction           
	 */
	/* find the direction recorded at row,col */
	lseek(dir_fd, (off_t) list->row * ncol * sizeof(DCELL), SEEK_SET);
	read(dir_fd, dir_buf, ncol * sizeof(DCELL));
	direction = *(dir_buf + list->col);
	neighbour = direction * 10;
	if (G_verbose() > 2)
	    G_message(_("direction read: %lf, neighbour found: %i"),
		      direction, neighbour);
	switch (neighbour) {
	case 225: /* ENE */
	    next_row = list->row - 1;
	    next_col = list->col + 2;
	    break;
	case 450: /* NE */
	    next_row = list->row - 1;
	    next_col = list->col + 1;
	    break;
	case 675: /* NNE */
	    next_row = list->row - 2;
	    next_col = list->col + 1;
	    break;
	case 900: /* N */
	    next_row = list->row - 1;
	    next_col = list->col;
	    break;
	case 1125: /* NNW */
	    next_row = list->row - 2;
	    next_col = list->col - 1;
	    break;
	case 1350: /* NW */
	    next_col = list->col - 1;
	    next_row = list->row - 1;
	    break;
	case 1575: /* WNW */
	    next_col = list->col - 2;
	    next_row = list->row - 1;
	    break;
	case 1800: /* W*/
	    next_row = list->row;
	    next_col = list->col - 1;
	    break;
	case 2025: /* WSW */
	    next_row = list->row + 1;
	    next_col = list->col - 2;
	    break;
	case 2250: /* SW */
	    next_row = list->row + 1;
	    next_col = list->col - 1;
	    break;
	case 2475: /* SSW */
	    next_row = list->row + 2;
	    next_col = list->col - 1;
	    break;
	case 2700: /* S */
	    next_row = list->row + 1;
	    next_col = list->col;
	    break;
	case 2925: /* SSE */
	    next_row = list->row + 2;
	    next_col = list->col + 1;
	    break;
	case 3150: /* SE */
	    next_row = list->row + 1;
	    next_col = list->col + 1;
	    break;
	case 3375: /* ESE */
	    next_row = list->row + 1;
	    next_col = list->col + 2;
	    break;
	case 3600: /* E */
	    next_row = list->row;
	    next_col = list->col + 1;
	    break;
	    /* default:
	       break;
	       Should probably add something here for the possibility of a non-direction map
	       G_fatal_error(_("Invalid direction given (possibly not a direction map)")); */
	}			/* end switch/case */

	if (next_col >= 0 && next_col < ncol && next_row >= 0 &&
	    next_row < nrow) {
	    list->next = (struct point *)G_malloc(sizeof(struct point));
	    list = list->next;
	    list->row = next_row;
	    list->col = next_col;
	    next_row = -1;
	    next_col = -1;
	    go = 1;
	}

    }				/* end while */

    /* allocate and fill the end-of-path flag */
    list->next = (struct point *)G_malloc(sizeof(struct point));
    list = list->next;
    list->row = INT_MAX;

    /* return a pointer to an empty structure */
    list->next = (struct point *)G_malloc(sizeof(struct point));
    list = list->next;
    list->next = NULL;

    G_free(dir_buf);

    return list;
}
