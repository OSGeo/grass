
/****************************************************************************
 *
 * MODULE:       v.surf.idw
 * AUTHOR(S):    Michael Shapiro, U.S. Army Construction Engineering Research Laboratory
 *               Improved algorithm (indexes points according to cell and ignores
 *               points outside current region) by Paul Kelly
 *               further: Radim Blazek <radim.blazek gmail.com>,  Huidae Cho <grass4u gmail.com>,
 *               Glynn Clements <glynn gclements.plus.com>, Markus Neteler <neteler itc.it>
 *               OGR support by Martin Landa <landa.martin gmail.com>
 * PURPOSE:      Surface interpolation from vector point data by Inverse
 *               Distance Squared Weighting
 * COPYRIGHT:    (C) 2003-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "proto.h"

int search_points = 12;

long npoints = 0;
long **npoints_currcell;
int nsearch;
static int i;

struct Point
{
    double north, east;
    double z;
};
struct list_Point
{
    double north, east;
    double z;
    double dist;
};
struct Point ***points;
struct Point *noidxpoints = NULL;
struct list_Point *list;
static struct Cell_head window;

int main(int argc, char *argv[])
{
    int fd, maskfd;
    CELL *mask;
    DCELL *dcell;
    struct GModule *module;
    struct History history;
    int row, col;
    int searchrow, searchcolumn, pointsfound;
    int *shortlistrows = NULL, *shortlistcolumns = NULL;
    long ncells = 0;
    double north, east;
    double dist;
    double sum1, sum2, interp_value;
    int n;
    double p;
    struct
    {
	struct Option *input, *npoints, *power, *output, *dfield, *col;
    } parm;
    struct
    {
        struct Flag *noindex;
    } flag;
    struct cell_list
    {
	int row, column;
	struct cell_list *next;
    };
    struct cell_list **search_list = NULL, **search_list_start = NULL;
    int max_radius, radius;
    int searchallpoints = 0;
    char *tmpstr1, *tmpstr2;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("interpolation"));
    G_add_keyword(_("IDW"));
    module->description =
	_("Provides surface interpolation from vector point data by Inverse "
	  "Distance Squared Weighting.");

    parm.input = G_define_standard_option(G_OPT_V_INPUT);

    parm.dfield = G_define_standard_option(G_OPT_V_FIELD);
    
    parm.col = G_define_standard_option(G_OPT_DB_COLUMN);
    parm.col->required = NO;
    parm.col->label = _("Name of attribute column with values to interpolate");
    parm.col->description = _("If not given and input is 2D vector map then category values are used. "
                               "If input is 3D vector map then z-coordinates are used.");
    parm.col->guisection = _("Values");

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);

    parm.npoints = G_define_option();
    parm.npoints->key = "npoints";
    parm.npoints->key_desc = "count";
    parm.npoints->type = TYPE_INTEGER;
    parm.npoints->required = NO;
    parm.npoints->description = _("Number of interpolation points");
    parm.npoints->answer = "12";
    parm.npoints->guisection = _("Settings");

    parm.power = G_define_option();
    parm.power->key = "power";
    parm.power->type = TYPE_DOUBLE;
    parm.power->answer = "2.0";
    parm.power->label = _("Power parameter");
    parm.power->description = 
    	_("Greater values assign greater influence to closer points");
    parm.power->guisection = _("Settings");

    flag.noindex = G_define_flag();
    flag.noindex->key = 'n';
    flag.noindex->label = _("Don't index points by raster cell");
    flag.noindex->description = _("Slower but uses"
				  " less memory and includes points from outside region"
				  " in the interpolation");
    flag.noindex->guisection = _("Settings");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (sscanf(parm.npoints->answer, "%d", &search_points) != 1 ||
	search_points < 1)
	G_fatal_error(_("Illegal number (%s) of interpolation points"),
		      parm.npoints->answer);
    
    list =
	(struct list_Point *) G_calloc((size_t) search_points,
				       sizeof(struct list_Point));

    p = atof(parm.power->answer);

    /* get the window, dimension arrays */
    G_get_window(&window);

    if (!flag.noindex->answer) {
	npoints_currcell = (long **)G_malloc(window.rows * sizeof(long *));
	points =
	    (struct Point ***)G_malloc(window.rows * sizeof(struct Point **));


	for (row = 0; row < window.rows; row++) {
	    npoints_currcell[row] =
		(long *)G_malloc(window.cols * sizeof(long));
	    points[row] =
		(struct Point **)G_malloc(window.cols *
					  sizeof(struct Point *));

	    for (col = 0; col < window.cols; col++) {
		npoints_currcell[row][col] = 0;
		points[row][col] = NULL;
	    }
	}
    }

    /* read the elevation points from the input sites file */
    read_sites(parm.input->answer, parm.dfield->answer,
	       parm.col->answer, flag.noindex->answer);
    
    if (npoints == 0)
	G_fatal_error(_("No points found"));
    nsearch = npoints < search_points ? npoints : search_points;

    if (!flag.noindex->answer) {
	/* Arbitrary point to switch between searching algorithms. Could do
	 * with refinement PK */
	if ((window.rows * window.cols) / npoints > 400) {
	    /* Using old algorithm.... */
	    searchallpoints = 1;
	    ncells = 0;

	    /* Make an array to contain the row and column indices that have
	     * sites in them; later will just search through all these. */
	    for (searchrow = 0; searchrow < window.rows; searchrow++)
		for (searchcolumn = 0; searchcolumn < window.cols;
		     searchcolumn++)
		    if (npoints_currcell[searchrow][searchcolumn] > 0) {
			shortlistrows = (int *)G_realloc(shortlistrows,
							 (1 +
							  ncells) *
							 sizeof(int));
			shortlistcolumns =
			    (int *)G_realloc(shortlistcolumns,
					     (1 + ncells) * sizeof(int));
			shortlistrows[ncells] = searchrow;
			shortlistcolumns[ncells] = searchcolumn;
			ncells++;
		    }
	}
	else {
	    /* Fill look-up table of row and column offsets for
	     * doing a circular region growing search looking for sites */
	    /* Use units of column width */
	    max_radius = (int)(0.5 + sqrt(window.cols * window.cols +
					  (window.rows * window.ns_res /
					   window.ew_res) * (window.rows *
							     window.ns_res /
							     window.ew_res)));

	    search_list =
		(struct cell_list **)G_malloc(max_radius *
					      sizeof(struct cell_list *));
	    search_list_start =
		(struct cell_list **)G_malloc(max_radius *
					      sizeof(struct cell_list *));

	    for (radius = 0; radius < max_radius; radius++)
		search_list[radius] = NULL;

	    for (row = 0; row < window.rows; row++)
		for (col = 0; col < window.cols; col++) {
		    radius = (int)sqrt(col * col +
				       (row * window.ns_res / window.ew_res) *
				       (row * window.ns_res / window.ew_res));
		    if (search_list[radius] == NULL)
			search_list[radius] =
			    search_list_start[radius] =
			    G_malloc(sizeof(struct cell_list));
		    else
			search_list[radius] =
			    search_list[radius]->next =
			    G_malloc(sizeof(struct cell_list));

		    search_list[radius]->row = row;
		    search_list[radius]->column = col;
		    search_list[radius]->next = NULL;
		}
	}
    }

    /* allocate buffers, etc. */

    dcell = Rast_allocate_d_buf();

    if ((maskfd = Rast_maskfd()) >= 0)
	mask = Rast_allocate_c_buf();
    else
	mask = NULL;


    fd = Rast_open_new(parm.output->answer, DCELL_TYPE);

    /* GTC Count of window rows */
    G_asprintf(&tmpstr1, n_("%d row", "%d rows", window.rows), window.rows);
    /* GTC Count of window columns */
    G_asprintf(&tmpstr2, n_("%d column", "%d columns", window.cols), window.cols);
    /* GTC First argument is map name, second - message about number of rows, third - columns. */
    G_important_message(_("Interpolating raster map <%s> (%s, %s)..."),
			parm.output->answer, tmpstr1, tmpstr2);
    G_free(tmpstr1);
    G_free(tmpstr2);

    north = window.north + window.ns_res / 2.0;
    for (row = 0; row < window.rows; row++) {
	G_percent(row, window.rows, 1);

	if (mask)
	    Rast_get_c_row(maskfd, mask, row);

	north -= window.ns_res;
	east = window.west - window.ew_res / 2.0;
	for (col = 0; col < window.cols; col++) {
	    east += window.ew_res;
	    /* don't interpolate outside of the mask */
	    if (mask && mask[col] == 0) {
		Rast_set_d_null_value(&dcell[col], 1);
		continue;
	    }

	    /* If current cell contains more than nsearch points just average
	     * all the points in this cell and don't look in any others */

	    if (!(flag.noindex->answer) && npoints_currcell[row][col] >= nsearch) {
		sum1 = 0.0;
		for (i = 0; i < npoints_currcell[row][col]; i++)
		    sum1 += points[row][col][i].z;

		interp_value = sum1 / npoints_currcell[row][col];
	    }
	    else {
		if (flag.noindex->answer)
		    calculate_distances_noindex(north, east);
		else {
		    pointsfound = 0;
		    i = 0;

		    if (searchallpoints == 1) {
			/* If there aren't many sites just check them all to find
			 * the nearest */
			for (n = 0; n < ncells; n++)
			    calculate_distances(shortlistrows[n],
						shortlistcolumns[n], north,
						east, &pointsfound);
		    }
		    else {
			radius = 0;
			while (pointsfound < nsearch) {
			    /* Keep widening the search window until we find
			     * enough points */
			    search_list[radius] = search_list_start[radius];
			    while (search_list[radius] != NULL) {
				/* Always */
				if (row <
				    (window.rows - search_list[radius]->row)
				    && col <
				    (window.cols -
				     search_list[radius]->column)) {
				    searchrow =
					row + search_list[radius]->row;
				    searchcolumn =
					col + search_list[radius]->column;
				    calculate_distances(searchrow,
							searchcolumn, north,
							east, &pointsfound);
				}

				/* Only if at least one offset is not 0 */
				if ((search_list[radius]->row > 0 ||
				     search_list[radius]->column > 0) &&
				    row >= search_list[radius]->row &&
				    col >= search_list[radius]->column) {
				    searchrow =
					row - search_list[radius]->row;
				    searchcolumn =
					col - search_list[radius]->column;
				    calculate_distances(searchrow,
							searchcolumn, north,
							east, &pointsfound);
				}

				/* Only if both offsets are not 0 */
				if (search_list[radius]->row > 0 &&
				    search_list[radius]->column > 0) {
				    if (row <
					(window.rows -
					 search_list[radius]->row) &&
					col >= search_list[radius]->column) {
					searchrow =
					    row + search_list[radius]->row;
					searchcolumn =
					    col - search_list[radius]->column;
					calculate_distances(searchrow,
							    searchcolumn,
							    north, east,
							    &pointsfound);
				    }
				    if (row >= search_list[radius]->row &&
					col <
					(window.cols -
					 search_list[radius]->column)) {
					searchrow =
					    row - search_list[radius]->row;
					searchcolumn =
					    col + search_list[radius]->column;
					calculate_distances(searchrow,
							    searchcolumn,
							    north, east,
							    &pointsfound);
				    }
				}

				search_list[radius] =
				    search_list[radius]->next;
			    }
			    radius++;
			}
		    }
		}

		/* interpolate */
		sum1 = 0.0;
		sum2 = 0.0;
		for (n = 0; n < nsearch; n++) {
		    if ((dist = sqrt(list[n].dist))) {
			sum1 += list[n].z / pow(dist, p);
			sum2 += 1.0 / pow(dist, p);
		    }
		    else {
			/* If one site is dead on the centre of the cell, ignore
			 * all the other sites and just use this value. 
			 * (Unlikely when using floating point numbers?) */
			sum1 = list[n].z;
			sum2 = 1.0;
			break;
		    }
		}
		interp_value = sum1 / sum2;
	    }
	    dcell[col] = (DCELL) interp_value;
	}
	Rast_put_d_row(fd, dcell);
    }
    G_percent(1, 1, 1);

    Rast_close(fd);

    /* writing history file */
    Rast_short_history(parm.output->answer, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(parm.output->answer, &history);
    
    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}

void newpoint(double z, double east, double north, int noindex)
{
    int row, column;

    row = (int)((window.north - north) / window.ns_res);
    column = (int)((east - window.west) / window.ew_res);

    if (!noindex) {
	if (row < 0 || row >= window.rows || column < 0 ||
	    column >= window.cols) ;
	else {			/* Ignore sites outside current region as can't be indexed */

	    points[row][column] =
		(struct Point *)G_realloc(points[row][column],
					  (1 +
					   npoints_currcell[row][column]) *
					  sizeof(struct Point));
	    points[row][column][npoints_currcell[row][column]].north = north;
	    points[row][column][npoints_currcell[row][column]].east = east;
	    points[row][column][npoints_currcell[row][column]].z = z;
	    npoints_currcell[row][column]++;
	    npoints++;
	}
    }
    else {
	noidxpoints = (struct Point *)G_realloc(noidxpoints,
						(1 +
						 npoints) *
						sizeof(struct Point));
	noidxpoints[npoints].north = north;
	noidxpoints[npoints].east = east;
	noidxpoints[npoints].z = z;
	npoints++;
    }
}

void calculate_distances(int row, int column, double north,
			 double east, int *pointsfound)
{
    int j, n;
    static int max;
    double dx, dy, dist;
    static double maxdist;

    /* Check distances and find the points to use in interpolation */
    for (j = 0; j < npoints_currcell[row][column]; j++) {
	/* fill list with first nsearch points */
	if (i < nsearch) {
	    dy = points[row][column][j].north - north;
	    dx = points[row][column][j].east - east;
	    list[i].dist = dy * dy + dx * dx;
	    list[i].z = points[row][column][j].z;
	    i++;

	    /* find the maximum distance */
	    if (i == nsearch) {
		maxdist = list[max = 0].dist;
		for (n = 1; n < nsearch; n++) {
		    if (maxdist < list[n].dist)
			maxdist = list[max = n].dist;
		}
	    }
	}
	else {

	    /* go through rest of the points now */
	    dy = points[row][column][j].north - north;
	    dx = points[row][column][j].east - east;
	    dist = dy * dy + dx * dx;

	    if (dist < maxdist) {
		/* replace the largest dist */
		list[max].z = points[row][column][j].z;
		list[max].dist = dist;
		maxdist = list[max = 0].dist;
		for (n = 1; n < nsearch; n++) {
		    if (maxdist < list[n].dist)
			maxdist = list[max = n].dist;
		}
	    }

	}
    }
    *pointsfound += npoints_currcell[row][column];
}

void calculate_distances_noindex(double north, double east)
{
    int n, max;
    double dx, dy, dist;
    double maxdist;

    /* fill list with first nsearch points */
    for (i = 0; i < nsearch; i++) {
	dy = noidxpoints[i].north - north;
	dx = noidxpoints[i].east - east;
	list[i].dist = dy * dy + dx * dx;
	list[i].z = noidxpoints[i].z;
    }
    /* find the maximum distance */
    maxdist = list[max = 0].dist;
    for (n = 1; n < nsearch; n++) {
	if (maxdist < list[n].dist)
	    maxdist = list[max = n].dist;
    }
    /* go through rest of the points now */
    for (; i < npoints; i++) {
	dy = noidxpoints[i].north - north;
	dx = noidxpoints[i].east - east;
	dist = dy * dy + dx * dx;

	if (dist < maxdist) {
	    /* replace the largest dist */
	    list[max].z = noidxpoints[i].z;
	    list[max].dist = dist;
	    maxdist = list[max = 0].dist;
	    for (n = 1; n < nsearch; n++) {
		if (maxdist < list[n].dist)
		    maxdist = list[max = n].dist;
	    }
	}
    }

}
