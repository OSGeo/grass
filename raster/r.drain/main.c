
/****************************************************************************
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

/* This is the original attribution from r.drain */

/************************************************************************ 
*                                                                       *
*      This is the main program for tracing out the path that a         *
*      drop of water would take if released at a certain location       *
*      on an input elevation map.  The program was written by           *
*      Kewan Q. Khawaja                                                 *
*      kewan@techlogix.com                                              *
*                                                                       *
* update to FP (2000): Pierre de Mouveaux <pmx@audiovu.com><pmx@free.fr>*
* bugfix in FCELL, DCELL: Markus Neteler 12/2000                        *
*************************************************************************/

/************************************************************************
*      Rewritten by Roger Miller 7/2001 based on subroutines from       *
*      r.fill.dir and on the original r.drain.                          *
*                                                                       *
*      24 July 2004: WebValley 2004, error checking and vector points added by *
*               Matteo Franchi          Liceo Leonardo Da Vinci Trento  *
*               Roberto Flor            ITC-irst                        *
*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* for using the "open" statement */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

/* for using the close statement */
#include <unistd.h>

#include <grass/gis.h>
#include <grass/site.h>
#include <grass/glocale.h>

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

    int fe, fd;
    int i, have_points = 0;
    int new_id;
    int nrows, ncols, points_row[MAX_POINTS], points_col[MAX_POINTS], npoints;
    int cell_open(), cell_open_new();
    int map_id;
    char map_name[GNAME_MAX], *map_mapset, new_map_name[GNAME_MAX];
    char *tempfile1, *tempfile2;
    char *search_mapset;
    struct History history;

    struct Cell_head window;
    struct Option *opt1, *opt2, *coordopt, *vpointopt;
    struct Flag *flag1, *flag2, *flag3;
    struct GModule *module;
    int in_type;
    void *in_buf;
    CELL *out_buf;
    struct band3 bnd, bndC;
    struct metrics *m = NULL;

    struct point *list;
    struct point *thispoint;
    int ival, bsz, start_row, start_col, mode;
    double east, north, val;
    struct point *drain(int, struct point *, int, int);
    int bsort(int, struct point *);


    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Traces a flow through an elevation model on a raster map layer.");

    opt1 = G_define_standard_option(G_OPT_R_INPUT);
    opt1->description =
	_("Name of existing raster map containing elevation surface");

    opt2 = G_define_standard_option(G_OPT_R_OUTPUT);
    opt2->description = _("Output drain raster map");

    coordopt = G_define_option();
    coordopt->key = "coordinate";
    coordopt->type = TYPE_STRING;
    coordopt->required = NO;
    coordopt->multiple = YES;
    coordopt->key_desc = "x,y";
    coordopt->description =
	_("The E and N coordinates of starting point(s)");

    vpointopt = G_define_option();
    vpointopt->key = "vector_points";
    vpointopt->type = TYPE_STRING;
    vpointopt->required = NO;
    vpointopt->multiple = YES;
    vpointopt->gisprompt= "old,vector,vector";
    vpointopt->key_desc = "name";
    vpointopt->description =
	_("Vector map(s) containing starting point(s)");

    flag1 = G_define_flag();
    flag1->key = 'c';
    flag1->description = _("Copy input cell values on output");

    flag2 = G_define_flag();
    flag2->key = 'a';
    flag2->description = _("Accumulate input values along the path");

    flag3 = G_define_flag();
    flag3->key = 'n';
    flag3->description = _("Count cell numbers along the path");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    strcpy(map_name, opt1->answer);
    strcpy(new_map_name, opt2->answer);

    /* get the name of the elevation map layer for filling */
    map_mapset = G_find_cell(map_name, "");
    if (!map_mapset)
	G_fatal_error(_("Raster map <%s> not found"), map_name);

    /*      allocate cell buf for the map layer */
    in_type = G_raster_map_type(map_name, map_mapset);

    /* set the pointers for multi-typed functions */
    set_func_pointers(in_type);

    if( (flag1->answer + flag2->answer + flag3->answer) > 1 )
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
    nrows = G_window_rows();
    ncols = G_window_cols();

    /* calculate true cell resolution */
    m = (struct metrics *)G_malloc(nrows * sizeof(struct metrics));

    if (m == NULL)
	G_fatal_error(_("Metrics allocation"));
    npoints = 0;
    if (coordopt->answer) {
	for (i = 0; coordopt->answers[i] != NULL; i += 2) {
	    G_scan_easting(coordopt->answers[i], &east, G_projection());
	    G_scan_northing(coordopt->answers[i + 1], &north, G_projection());
	    start_col = (int)G_easting_to_col(east, &window);
	    start_row = (int)G_northing_to_row(north, &window);

	    if (start_row < 0 || start_row > nrows ||
		start_col < 0 || start_col > ncols) {
		G_warning(_("Starting point %d is outside the current region"),
			  i + 1);
		continue;
	    }
	    points_row[npoints] = start_row;
	    points_col[npoints] = start_col;
	    npoints++;
	    if(npoints >= MAX_POINTS) G_fatal_error(_("Too many start points"));
	    have_points = 1;
	}
    }
    if (vpointopt->answer) {
	for (i = 0; vpointopt->answers[i] != NULL; i++) {
	    FILE *fp;
	    /* struct start_pt  *new_start_pt; */
	    Site *site = NULL;	/* pointer to Site */
	    int dims, strs, dbls;
	    RASTER_MAP_TYPE cat;

	    search_mapset = G_find_sites(vpointopt->answers[i], "");
	    if (search_mapset == NULL)
		G_fatal_error(_("Vector map <%s> not found"),
			      vpointopt->answers[i]);

	    fp = G_fopen_sites_old(vpointopt->answers[i], search_mapset);

	    if (0 != G_site_describe( fp, &dims, &cat, &strs, &dbls))
		G_fatal_error(_("Failed to guess site file format"));

	    site = G_site_new_struct(cat, dims, strs, dbls);

	    for (; (G_site_get(fp, site) != EOF);) {
		if (!G_site_in_region(site, &window))
		    continue;

		start_col = (int)G_easting_to_col(site->east, &window);
		start_row = (int)G_northing_to_row(site->north, &window);

		/* effectively just a duplicate check to G_site_in_region() ??? */
		if (start_row < 0 || start_row > nrows || start_col < 0 || start_col > ncols)
		    continue;

		points_row[npoints] = start_row;
		points_col[npoints] = start_col;
		npoints++;
		if(npoints >= MAX_POINTS) G_fatal_error(_("Too many start points"));
		have_points = 1;
	    }

	    /* only catches maps out of range until something is found, not after */
	    if(!have_points) {
		G_warning(_("Starting vector map <%s> contains no points in the current region"),
		      vpointopt->answers[i]);
	    }
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
    map_id = G_open_cell_old(map_name, map_mapset);

    /* get some temp files */
    tempfile1 = G_tempfile();
    tempfile2 = G_tempfile();

    fe = open(tempfile1, O_RDWR | O_CREAT);
    fd = open(tempfile2, O_RDWR | O_CREAT);

    /* transfer the input map to a temp file */
    for (i = 0; i < nrows; i++) {
	get_row(map_id, in_buf, i);
	write(fe, in_buf, bnd.sz);
    }
    G_close_cell(map_id);

    /* fill one-cell pits and take a first stab at flow directions */
    filldir(fe, fd, nrows, &bnd, m);

    /* determine flow directions for more ambiguous cases */
    resolve(fd, nrows, &bndC);

    /* free the buffers already used */
    G_free (bndC.b[0]);
    G_free (bndC.b[1]);
    G_free (bndC.b[2]);

    G_free (bnd.b[0]);
    G_free (bnd.b[1]);
    G_free (bnd.b[2]);

    /* determine the drainage paths */

    /* repeat for each starting point */
    for (i = 0; i < npoints; i++) {
	/* use the flow directions to determine the drainage path
	 * results are compiled as a linked list of points in downstream order */
	thispoint->row = points_row[i];
	thispoint->col = points_col[i];
	thispoint->next = NULL;
	thispoint = drain(fd, thispoint, nrows, ncols);
    }

    /* do the output */

    if (mode == 0 || mode == 3) {

	/* Output will be a cell map */
	/* open a new file and allocate an output buffer */
	new_id = G_open_cell_new(new_map_name);
	out_buf = G_allocate_c_raster_buf();

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
	for (i = 0; i < nrows; i++) {
	    G_set_c_null_value(out_buf, ncols);
	    thispoint = list;
	    while (thispoint->next != NULL) {
		if (thispoint->row == i)
		    out_buf[thispoint->col] = (int)thispoint->value;
		thispoint = thispoint->next;
	    }
	    G_put_c_raster_row(new_id, out_buf);

	}
    }
    else {			/* mode = 1 or 2 */


	/* Output will be of the same type as input */
	/* open a new file and allocate an output buffer */
	new_id = G_open_raster_new(new_map_name, in_type);
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
	for (i = 0; i < nrows; i++) {
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
    }

    /* close files and free buffers */
    G_close_cell(new_id);

    G_put_cell_title (new_map_name, "Surface flow trace");

    G_short_history(new_map_name, "raster", &history);
    G_command_history(&history);
    G_write_history(new_map_name, &history);

    close(fe);
    close(fd);

    unlink(tempfile1);
    unlink(tempfile2);
    G_free(in_buf);
    G_free(out_buf);

    exit(EXIT_SUCCESS);
}

struct point *drain(int fd, struct point *list, int nrow, int ncol)
{
    int go = 1, next_row, next_col;
    CELL direction;
    CELL *dir;

    dir = G_allocate_c_raster_buf();
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
