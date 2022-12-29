
/****************************************************************************
 *
 * MODULE:       r.contour
 *
 * AUTHOR(S):    Terry Baker - CERL
 *               Andrea Aime <aaime liberto it>
 *
 * PURPOSE:      Produces a vector map of specified contours from a
 *               raster map layer.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/
/* Algorithm comment from Jim Westervelt:
   It appears that the code is doing a linear interpolation between cells and
   finding where a given contour crosses through each cell.  In the end,
   strings of coordinates are generated for user selected contours.  In GRASS,
   these are lines and not necessarily polygons.
 */

#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local_proto.h"


struct cell
{
    DCELL z[4];
    int r, c;
    int edge;
};

static int getnewcell(struct cell *, int, int, DCELL **);
static void newedge(struct cell *);
static int findcrossing(struct cell *, double,
			struct Cell_head, struct line_pnts *, int *);
static void getpoint(struct cell *curr, double,
		     struct Cell_head, struct line_pnts *);


void contour(double levels[],
	     int nlevels,
	     struct Map_info Map,
	     DCELL ** z, struct Cell_head Cell, int n_cut)
{
    int nrow, ncol;		/* number of rows and columns in current region */
    int startrow, startcol;	/* start row and col of current line */
    int n, i, j;		/* loop counters */
    double level;		/* current contour level */
    char **hit;			/* array of flags--1 if Cell has been hit;  */

    /*  0 if Cell is still to be checked */
    struct line_pnts *Points;
    struct line_cats *Cats;
    int outside;		/* 1 if line is exiting region; 0 otherwise */
    struct cell current;
    int p1, p2;			/* indexes to end points of cell edges */

    int ncrossing;		/* number of found crossing */

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nrow = Cell.rows;
    ncol = Cell.cols;

    hit = (char **)G_malloc((nrow - 1) * sizeof(char *));
    for (i = 0; i < nrow - 1; i++)
	hit[i] = (char *)G_malloc((ncol - 1) * sizeof(char));

    ncrossing = 0;

    G_message(n_("Writing vector contour (one level)...", 
        "Writing vector contours (total levels %d)...", nlevels), nlevels);

    for (n = 0; n < nlevels; n++) {
	level = levels[n];
	G_percent(n+1, nlevels, 2);	/* print progress */

	/* initialize hit array */
	for (i = 0; i < nrow - 1; i++) {
	    for (j = 0; j < ncol - 1; j++) {
		hit[i][j] = 0;
	    }
	}

	/* check each cell of top and bottom borders  */
	for (startrow = 0; startrow <= nrow - 2; startrow += (nrow - 2)) {
	    for (startcol = 0; startcol <= ncol - 2; startcol++) {

		/* look for starting point of new line */
		if (!hit[startrow][startcol]) {
		    current.r = startrow;
		    current.c = startcol;

		    /* is this top or bottom? */
		    if (startrow < nrow - 2)	/* top */
			current.edge = 0;
		    else	/* bottom edge */
			current.edge = 2;

		    outside = getnewcell(&current, nrow, ncol, z);

		    p1 = current.edge;
		    p2 = current.edge + 1;

		    if (checkedge(current.z[p1], current.z[p2], level)) {
			getpoint(&current, level, Cell, Points);
			/* while not off an edge, follow line */
			while (!outside) {
			    hit[current.r][current.c] |=
				findcrossing(&current, level, Cell, Points,
					     &ncrossing);
			    newedge(&current);
			    outside = getnewcell(&current, nrow, ncol, z);
			}
			if ((n_cut <= 0) || ((Points->n_points) >= n_cut)) {
			    Vect_reset_cats(Cats);
			    Vect_cat_set(Cats, 1, n + 1);
			    Vect_write_line(&Map, GV_LINE, Points, Cats);
			}
			Vect_reset_line(Points);
		    }		/* if checkedge */
		}		/* if ! hit */
	    }			/* for columns */
	}			/* for rows */

	/* check right and left borders (each row of first and last column) */
	for (startcol = 0; startcol <= ncol - 2 ; startcol += (ncol - 2)) {
	    for (startrow = 0; startrow <= nrow - 2; startrow++) {
		/* look for starting point of new line */
		if (!hit[startrow][startcol]) {
		    current.r = startrow;
		    current.c = startcol;

		    /* is this left or right edge? */
		    if (startcol < ncol - 2)	/* left */
			current.edge = 3;
		    else	/* right edge */
			current.edge = 1;

		    outside = getnewcell(&current, nrow, ncol, z);

		    p1 = current.edge;
		    p2 = (current.edge + 1) % 4;
		    if (checkedge(current.z[p1], current.z[p2], level)) {
			getpoint(&current, level, Cell, Points);
			/* while not off an edge, follow line */
			while (!outside) {
			    hit[current.r][current.c] |=
				findcrossing(&current, level, Cell, Points,
					     &ncrossing);
			    newedge(&current);
			    outside = getnewcell(&current, nrow, ncol, z);
			}
			if ((n_cut <= 0) || ((Points->n_points) >= n_cut)) {
			    Vect_reset_cats(Cats);
			    Vect_cat_set(Cats, 1, n + 1);
			    Vect_write_line(&Map, GV_LINE, Points, Cats);
			}
			Vect_reset_line(Points);
		    }		/* if checkedge */
		}		/* if ! hit */
	    }			/* for rows */
	}			/* for columns */

	/* check each interior Cell */
	for (startrow = 1; startrow <= nrow - 3; startrow++) {
	    for (startcol = 1; startcol <= ncol - 3; startcol++) {
		/* look for starting point of new line */
		if (!hit[startrow][startcol]) {
		    current.r = startrow;
		    current.c = startcol;
		    current.edge = 0;
		    p1 = current.edge;
		    p2 = current.edge + 1;

		    outside = getnewcell(&current, nrow, ncol, z);
		    if (!outside &&
			checkedge(current.z[p1], current.z[p2], level)) {
			getpoint(&current, level, Cell, Points);
			hit[current.r][current.c] |=
			    findcrossing(&current, level, Cell, Points,
					 &ncrossing);
			newedge(&current);
			outside = getnewcell(&current, nrow, ncol, z);

			/* while not back to starting point, follow line */
			while (!outside &&
			       ((current.edge != 0) ||
				((current.r != startrow) ||
				 (current.c != startcol)))) {
			    hit[current.r][current.c] |=
				findcrossing(&current, level, Cell, Points,
					     &ncrossing);
			    newedge(&current);
			    outside = getnewcell(&current, nrow, ncol, z);
			}
			if ((n_cut <= 0) || ((Points->n_points) >= n_cut)) {
			    Vect_reset_cats(Cats);
			    Vect_cat_set(Cats, 1, n + 1);
			    Vect_write_line(&Map, GV_LINE, Points, Cats);
			}
			Vect_reset_line(Points);
		    }		/* if checkedge */
		}		/* if ! hit */
	    }			/* for rows */
	}			/* for columns */
    }				/* for levels */

    if (ncrossing > 0) {
	G_warning(n_("%d crossing found", 
        "%d crossings found", 
        ncrossing), ncrossing);
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
}

/***************************************************************************
getnewcell
if cell is in range, finds data values of corner points of current cell
 returns 0
else returns 1
***************************************************************************/
static int getnewcell(struct cell *current, int nrow, int ncol, DCELL ** z)
{

    if ((current->r >= 0) && (current->r <= nrow - 2) &&
	(current->c >= 0) && (current->c <= ncol - 2)) {
	current->z[0] = z[current->r][current->c];
	current->z[1] = z[current->r][current->c + 1];
	current->z[2] = z[current->r + 1][current->c + 1];
	current->z[3] = z[current->r + 1][current->c];

	/* testing for NULL values here does not work */

	return 0;
    }
    return 1;
}

/*******************************************************************
newedge-updates edge number and row and col to those of next cell
*******************************************************************/
static void newedge(struct cell *current)
{
    switch (current->edge) {
    case 0:
	current->r -= 1;
	current->edge = 2;
	break;
    case 1:
	current->c += 1;
	current->edge = 3;
	break;
    case 2:
	current->r += 1;
	current->edge = 0;
	break;
    case 3:
	current->c -= 1;
	current->edge = 1;
	break;
    default:
	G_fatal_error(_("Illegal edge number"));
    }
}

/***********************************************************************
  findcrossing-- decides which edge exit point from cell is on and changes
  value of edge. 
  Returns 1 if only 2 crossings found ( don't want to check this Cell again);
  0 otherwise.
****************************************************************************/
static int findcrossing(struct cell *current, double level,
			struct Cell_head Cell, struct line_pnts *Points,
			int *ncrossing)
{
    int i, j;
    int numcross;		/* number of crossings found in this Cell */
    int edgehit[4];		/* hit flag for each edge of Cell */
    int cellhit = 0;
    double mid;

    numcross = 0;
    for (i = 0; i < 4; i++) {
	edgehit[i] = 0;
	edgehit[i] = checkedge(current->z[i], current->z[(i + 1) % 4], level);
	if (edgehit[i])
	    numcross++;
    }
    if (numcross == 2) {
	cellhit = 1;
	edgehit[current->edge] = 0;
	for (j = 0; j < 4; j++) {
	    if (edgehit[j]) {
		current->edge = j;
		getpoint(current, level, Cell, Points);
		break;
	    }
	}
    }
    else if (numcross == 4) {
	if (current->edge == 0)
	    cellhit = 1;

	mid =
	    (current->z[0] + current->z[1] + current->z[2] +
	     current->z[3]) / 4;
	if (checkedge(mid, current->z[current->edge], level))
	    current->edge = (current->edge == 0) ? 3 : current->edge - 1;
	else
	    current->edge = (current->edge == 3) ? 0 : current->edge + 1;
	getpoint(current, level, Cell, Points);

	if (current->edge == 0)
	    cellhit = 1;

    }
    else {
	if (1 == numcross) {
	    G_debug(1, "%d crossings in cell %d, %d",
		    numcross, current->r, current->c);
	    (*ncrossing)++;
	}

	cellhit = 1;
    }

    return cellhit;
}

/************************************************************************
getpoint-- finds crossing point using linear interpolation, 
	  converts from row-column to x-y space, and adds point to current  line.
************************************************************************/
static void getpoint(struct cell *curr, double level,
		     struct Cell_head Cell, struct line_pnts *Points)
{
    double x, y;
    double ratio;
    int p1, p2;

    p1 = curr->edge;
    p2 = (curr->edge + 1) % 4;
    if (Rast_raster_cmp(&curr->z[p1], &curr->z[p2], DCELL_TYPE) == 0)
	ratio = 1;
    else if (Rast_is_d_null_value(&curr->z[p1]))
	ratio = 0.5;
    else if (Rast_is_d_null_value(&curr->z[p2]))
	ratio = 0.5;
    else
	ratio = (level - curr->z[p1]) / (curr->z[p2] - curr->z[p1]);

    switch (curr->edge) {

    case 0:
	y = curr->r;
	x = curr->c + ratio;
	break;
    case 1:
	y = curr->r + ratio;
	x = curr->c + 1;
	break;
    case 2:
	y = curr->r + 1;
	x = curr->c + 1 - ratio;
	break;
    case 3:
	y = curr->r + 1 - ratio;
	x = curr->c;
	break;
    default:
	G_fatal_error(_("Edge number out of range"));
    }
    /* convert r/c values to x/y values */

    y = Cell.north - (y + .5) * Cell.ns_res;
    x = Cell.west + (x + .5) * Cell.ew_res;

    if (Points->n_points == 0 || (Points->n_points > 0 &&
        (Points->x[Points->n_points - 1] != x ||
	 Points->y[Points->n_points - 1] != y))) {
	Vect_append_point(Points, x, y, level);
    }
}

/***********************************************************************
checkedge--returns 1 if level is between values d1 & d2; 
		   0 otherwise.
*********************************************************************/
int checkedge(DCELL d1, DCELL d2, double level)
{
    if (((d1 <= level) && (d2 > level)) || ((d1 > level) && (d2 <= level)))
	return 1;

    return 0;
}

/*********************************************************************/
