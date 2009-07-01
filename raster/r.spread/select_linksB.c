
/************************************************************************
 *
 *	select_linksB.c		in ~/r.spread
 *
 *	function to select cell links for elliptical spread and to put 
 *	the destinated cell of a link in a simple linked list of type
 *	cell_ptrHa. 
 *
 *	The selection rule is: cells in an enlarged spread ellipse 
 *	centered at the current spread cell. The apogee is 1 cell plus 
 *	the integer number of cells of the ratio of the maximum rate 
 *	of spread (ROS) to the base (perpendicular to the max) ROS.
 *	
 *	By Jianping Xu, Rutgers University.
 *	06/22/93
 *
 *************************************************************************/

#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "costHa.h"
#include "cell_ptrHa.h"
#include "local_proto.h"

#ifndef PI
#define PI M_PI
#endif

#define DATA(map, r, c)		(map)[(r) * ncols + (c)]

/*#define DEBUG */

void select_linksB(struct costHa *pres_cell, int least, float comp_dens)
{
    extern CELL *map_max, *map_dir, *map_base;	/*for ellipse */
    extern CELL *map_visit;	/*for avoiding redundancy */
    extern int BARRIER;
    extern int nrows, ncols;
    extern struct cell_ptrHa *front_cell, *rear_cell;
    float angle;		/*that of a link cell to spread cell */
    float dir_angle;		/*that of the maximum ROS */
    float cos_dir_angle, sin_dir_angle;
    float polar_len;		/*polar distance of the ellipse */
    float distance;
    int n = 0, s = 0, e = 0, w = 0;	/*parameters defining a rectangule */
    int ros_max, ros_base, dir;	/*3 params defining an elps */
    int row, col;

#ifdef DEBUG
    printf("\nin select pres(%d,%d) ", pres_cell->row, pres_cell->col);
#endif
#ifdef DEBUG
    for (row = 0; row < nrows; row++) {
	printf("\nrow %d: ", row);
	for (col = 0; col < ncols; col++)
	    printf("%d ", DATA(map_base, row, col));
    }
#endif

    ros_max = DATA(map_max, pres_cell->row, pres_cell->col);
    ros_base = DATA(map_base, pres_cell->row, pres_cell->col);
    dir = DATA(map_dir, pres_cell->row, pres_cell->col);

    dir_angle = dir % 360 * PI / 180;
    sin_dir_angle = sin(dir_angle);
    cos_dir_angle = cos(dir_angle);

    /* identifies a rectangular just enclosing the ellipse,
     * thus avoiding redundant work in selection */

    if (dir_angle >= 7 * PI / 4 || dir_angle < PI / 4) {	/*down mainly N */
	n = (ros_max / ros_base - 1) * comp_dens + least;
	s = least;
	w = (ros_max / ros_base - 1) * comp_dens + least;
	e = (ros_max / ros_base - 1) * comp_dens + least;
    }
    if (dir_angle >= PI / 4 && dir_angle < 3 * PI / 4) {	/*down mainly E */
	n = (ros_max / ros_base - 1) * comp_dens + least;
	s = (ros_max / ros_base - 1) * comp_dens + least;
	w = least;
	e = (ros_max / ros_base - 1) * comp_dens + least;
    }
    if (dir_angle >= 3 * PI / 4 && dir_angle < 5 * PI / 4) {	/*down mainly S */
	n = least;
	s = (ros_max / ros_base - 1) * comp_dens + least;
	w = (ros_max / ros_base - 1) * comp_dens + least;
	e = (ros_max / ros_base - 1) * comp_dens + least;
    }
    if (dir_angle >= 5 * PI / 4 && dir_angle < 7 * PI / 4) {	/*down mainly W */
	n = (ros_max / ros_base - 1) * comp_dens + least;
	s = (ros_max / ros_base - 1) * comp_dens + least;
	w = (ros_max / ros_base - 1) * comp_dens + least;
	e = least;
    }

    if (n > least)
	n--;
    if (n > least)
	n--;
    if (s > least)
	s--;
    if (s > least)
	s--;
    if (e > least)
	e--;
    if (e > least)
	e--;
    if (w > least)
	w--;
    if (w > least)
	w--;

    /* collect cells in the elliptical templet, put into a list */
    for (row = pres_cell->row - n; row <= pres_cell->row + s; row++) {
	if (row < 0 || row >= nrows)	/*outside n,s */
	    continue;

	for (col = pres_cell->col - w; col <= pres_cell->col + e; col++) {

	    G_debug(4,
		"(%d, %d) max=%d base=%d dir=%d least=%d n=%d s=%d e=%d w=%d base=%d BARRIER=%d",
		 row, col, ros_max, ros_base, dir, least, n, s, e, w,
		 DATA(map_base, row, col), BARRIER);

	    if (col < 0 || col >= ncols)	/*outside e,w */
		continue;

	    G_debug(4,
		"(%d, %d) max=%d base=%d dir=%d least=%d n=%d s=%d e=%d w=%d base=%d BARRIER=%d",
		 row, col, ros_max, ros_base, dir, least, n, s, e, w,
		 DATA(map_base, row, col), BARRIER);

	    if (row == pres_cell->row && col == pres_cell->col)
		continue;	/*spread cell */

	    G_debug(4,
		"(%d, %d) max=%d base=%d dir=%d least=%d n=%d s=%d e=%d w=%d base=%d BARRIER=%d",
		 row, col, ros_max, ros_base, dir, least, n, s, e, w,
		 DATA(map_base, row, col), BARRIER);

	    if (DATA(map_visit, row, col))	/*visited? */
		continue;

	    G_debug(4,
		"(%d, %d) max=%d base=%d dir=%d least=%d n=%d s=%d e=%d w=%d base=%d BARRIER=%d",
		 row, col, ros_max, ros_base, dir, least, n, s, e, w,
		 DATA(map_base, row, col), BARRIER);

	    if (DATA(map_base, row, col) == BARRIER)	/*barriers */
		continue;

	     G_debug(4,
		"(%d, %d) max=%d base=%d dir=%d least=%d n=%d s=%d e=%d w=%d",
		 row, col, ros_max, ros_base, dir, least, n, s, e, w);
	    angle =
		atan2((double)(col - pres_cell->col),
		      (double)(pres_cell->row - row));

	    /*the polar (square) distance of enlarged ellipse */
	    polar_len =
		(1 / (1 - (1 - ros_base / (float)ros_max)
			* cos(angle - dir_angle))) *
			(1 / (1 - (1 - ros_base / (float)ros_max) *
			cos(angle - dir_angle))) + 2 * least * least;

	    /*the (square) distance to this cell */
	    distance =
		(float)(row - pres_cell->row) * (row - pres_cell->row) +
		(col - pres_cell->col) * (col - pres_cell->col);

	    /*applies the selection rule */
	    if (distance > polar_len)
		continue;

	    insert2Ha(&front_cell, &rear_cell, (float)angle, row, col);
	}
    }
}
