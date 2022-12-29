
/******************************************************************************
 *	spot.c 		in ~/r.spread.fast
 *	
 *  	This function is for wildfire spread simulation only.
 *	It 1) is an inverse square distance randomization of the maximum 
 * 	   spotting distance;
 *
 *		| p(X)
 *		|
 *		|*
 *		|*
 *		| 
 *		| *
 *		|  			1
 *		|   *		p(X) = ---	(0 <= X <= max_dist)
 *		|    *		       X^2
 *		|      *
 *		|	 *
 *		|	   *
 *		|	      *
 *		|		 *
 *		|		     *
 *		|			  *
 *		|______________________________********_______  X
 *						      ^
 *						     max_dist
 *
 * 	2) the maximum spotting distance is derived from simplification of 
 *	   Chase (1984);
 *	3) the landing firebrand may ignite spread based on fine fuel
 *	   moisture dictated probability simplified from Rothermel (1983);
 *	4) spotting travel time is proportional to but slower than windspeed;
 *	5) there is an elapsed time to reach equilibrium rate of spread (ROS).
 *	   This elapsed time is proportional to the ROS. 
 * Refs:
 * Chase, C. H., 1984, Spotting distance from wind-driven surface fires --
 * 	ententions of equations for pocket calculators, US Forest Service, Res. 
 *	Note INT-346, Ogden, Uhta, 27 p.
 * Rothermel, R. C., 1983, How to predict the spread and intensity of forest 
 *	and range fires. US Forest Service, Gen. Tech. Rep. INT-143. Ogden, 
 *	Utha, 161 p.
 ******************************************************************************/
#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "cmd_line.h"
#include "costHa.h"
#include "cell_ptrHa.h"
#include "local_proto.h"

#ifndef PI
#define PI M_PI
#endif

#define DATA(map, r, c)		(map)[(r) * ncols + (c)]

void spot(struct costHa *pres_cell, int dir /* direction of forward ROS */ )
{
    extern CELL *map_max;	/* max ROS (cm/min) */
    extern CELL *map_spotdist;	/* max spotting distance (m) */
    extern CELL *map_velocity;	/* midflame windspeed (ft/min) */
    extern CELL *map_mois;	/* fuel moisture (%) */

    /*      extern float    PI; */
    extern int nrows, ncols;
    extern struct Cell_head window;
    float spot_cost;		/* spotting travel time (min) */
    float min_cost;		/* min cumulative time (min) */
    float U;			/* wind speed at 6m (m/min) */
    float Te;			/* time to reach max ROS (min) */
    int land_dist;		/* stochastic landing dist (m) */
    int land_distc;		/* land_dist in cell counts */
    int row, col;

    /* Find the (cell) location spotting might reach */

    land_dist = pick_dist(DATA(map_spotdist, pres_cell->row, pres_cell->col));

    G_debug(1, "pres_cell(%d, %d): land_dist=%d", pres_cell->row,
	   pres_cell->col, land_dist);

    land_distc = land_dist / (window.ns_res / 100);	/* 100 fac due to cm */

    if (land_distc < 2)		/* no need for adjacent cells */
	return;
    row = pres_cell->row - land_distc * cos((dir % 360) * PI / 180) + 0.5;
    col = pres_cell->col + land_distc * sin((dir % 360) * PI / 180) + 0.5;
    if (row < 0 || row >= nrows)	/* outside the region */
	return;
    if (col < 0 || col >= ncols)	/* outside the region */
	return;
    if (DATA(map_max, row, col) <= 0)	/* a barrier */
	return;

    /* check if ignitable based on probs. modified from Rothermel (1983) */

    if (DATA(map_mois, row, col) > 17)	/* too wet */
	return;

    G_debug(1,
      "	pre pick_ignite(): land_distc(%d, %d)=%d dir=%d PI=%.2f (dir%%360)*PI/180=%.2f",
	  row, col, land_distc, dir, PI, (dir % 360) * PI / 180);

    if (pick_ignite(DATA(map_mois, row, col)) == 0)	/* not success */
	return;

    G_debug(1, "	post pick_ignite(): land_distc(%d, %d)=%d ",
	   row, col, land_distc);

    /* travel time by spotting */

    U = 0.305 * DATA(map_velocity, pres_cell->row, pres_cell->col);
    /*NOTE: use value at midflame */
    spot_cost = land_dist / U;

    /* elapsed time to reach the max ROS, proportional to ROS */

    Te = DATA(map_max, pres_cell->row, pres_cell->col) / 1000 + 1;

    /* cumulative travel time since start */

    min_cost = pres_cell->min_cost + spot_cost + Te;

    /* update it to the to_cell */
    G_debug(1, "		min_cost=%.2f: pres=%.2f spot=%.2f Te=%.2f",
	   min_cost, pres_cell->min_cost, spot_cost, Te);

    update(pres_cell, row, col, (double)dir, min_cost);
}
