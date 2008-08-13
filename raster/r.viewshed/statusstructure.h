/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu

 *               Ported to GRASS by William Richard -
 *               wkrichar@bowdoin.edu or willster3021@gmail.com
 *
 * Date:         july 2008 
 * 
 * PURPOSE: To calculate the viewshed (the visible cells in the
 * raster) for the given viewpoint (observer) location.  The
 * visibility model is the following: Two points in the raster are
 * considered visible to each other if the cells where they belong are
 * visible to each other.  Two cells are visible to each other if the
 * line-of-sight that connects their centers does not intersect the
 * terrain. The height of a cell is assumed to be constant, and the
 * terrain is viewed as a tesselation of flat cells.  This model is
 * suitable for high resolution rasters; it may not be accurate for
 * low resolution rasters, where it may be better to interpolate the
 * height at a point based on the neighbors, rather than assuming
 * cells are "flat".  The viewshed algorithm is efficient both in
 * terms of CPU operations and I/O operations. It has worst-case
 * complexity O(n lg n) in the RAM model and O(sort(n)) in the
 * I/O-model.  For the algorithm and all the other details see the
 * paper: "Computing Visibility on * Terrains in External Memory" by
 * Herman Haverkort, Laura Toma and Yi Zhuang.
 *
 * COPYRIGHT: (C) 2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 *****************************************************************************/


#ifndef _YZ__STATUSSTRUCTURE_H
#define _YZ__STATUSSTRUCTURE_H

/*
   This header file defines the status structure and related functions.
 */

#ifdef __GRASS__
#include <grass/config.h>
#endif

#include "grid.h"
#include "rbbst.h"
#include "visibility.h"

#define PI 3.1416

typedef struct statusnode_
{
    dimensionType row, col;	/*position of the cell */
    float elev;			/*elevation of cell */
    double dist2vp;		/*distance to the viewpoint */
    double gradient;		/*gradient of the Line of Sight */
} StatusNode;


typedef struct statuslist_
{
    RBTree *rbt;		/*pointer to the root of the bst */
} StatusList;



/* ------------------------------------------------------------ */

/*return an estimate of the size of active structure */
long long get_active_str_size_bytes(GridHeader * hd);


//given a StatusNode, fill in its dist2vp and gradient
void calculate_dist_n_gradient(StatusNode * sn, Viewpoint * vp);


/*create an empty status list. */
StatusList *create_status_struct();

void delete_status_structure(StatusList * sl);

/*returns true is it is empty */
int is_empty(StatusList * sl);


/*delete the statusNode with the given key */
void delete_from_status_struct(StatusList * sl, double dist2vp);

/*insert the element into the status structure */
void insert_into_status_struct(StatusNode sn, StatusList * sl);

/*find the node with max Gradient. The node must be
   //within the distance (from viewpoint) given */
double find_max_gradient_in_status_struct(StatusList * sl, double dist);

/*find the vertical angle in degrees between the viewpoint and the
  point represented by the StatusNode.  Assumes all values (except
  gradient) in sn have been filled.*/
float get_vertical_angle(Viewpoint vp, StatusNode sn, int doCurv);


#endif
