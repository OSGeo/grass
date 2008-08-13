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


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __GRASS__
extern "C"
{
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
}
#include "grass.h"
#endif

#include "statusstructure.h"


/*SMALLEST_GRADIENT is defined in rbbst.h */


/* ------------------------------------------------------------ */
/*find the vertical angle in degrees between the viewpoint and the
  point represented by the StatusNode.  Assumes all values (except
  gradient) in sn have been filled. The value returned is in [0,
  180]. if doCurv is set we need to consider the curvature of the
  earth */
float get_vertical_angle(Viewpoint vp, StatusNode sn, int doCurv) {
  
  /*determine the difference in elevation, based on the curvature*/
  float diffElev;
  diffElev = vp.elev - sn.elev;
  
  /*calculate and return the angle in degrees*/
  assert(fabs(sn.dist2vp) > 0.001); 
  if(diffElev >= 0.0)
    return (atan(sn.dist2vp / diffElev) * (180/PI));
  else
    return ((atan(fabs(diffElev) / sn.dist2vp) * (180/PI)) + 90);
}



/* ------------------------------------------------------------ */
/*return an estimate of the size of active structure */
long long get_active_str_size_bytes(GridHeader * hd)
{

  long long sizeBytes;
  
  printf("Estimated size active structure:");
  printf(" (key=%d, ptr=%d, total node=%d B)",
	 (int)sizeof(TreeValue),
	 (int)sizeof(TreeNode *), (int)sizeof(TreeNode));
  sizeBytes = sizeof(TreeNode) * max(hd->ncols, hd->nrows);
  printf(" Total= %lld B\n", sizeBytes);
  return sizeBytes;
}




/* ------------------------------------------------------------ */
/*given a StatusNode, fill in its dist2vp and gradient */
void calculate_dist_n_gradient(StatusNode * sn, Viewpoint * vp)
{

    assert(sn && vp);
    /*sqrt is expensive
       //sn->dist2vp = sqrt((float) ( pow(sn->row - vp->row,2.0) + 
       //               pow(sn->col - vp->col,2.0)));
       //sn->gradient = (sn->elev  - vp->elev)/(sn->dist2vp); */
    sn->dist2vp = (sn->row - vp->row) * (sn->row - vp->row) +
	(sn->col - vp->col) * (sn->col - vp->col);
    sn->gradient =
	(sn->elev - vp->elev) * (sn->elev - vp->elev) / (sn->dist2vp);
    /*maintain sign */
    if (sn->elev < vp->elev)
	sn->gradient = -sn->gradient;
    return;
}




/* ------------------------------------------------------------ */
/*create an empty  status list */
StatusList *create_status_struct()
{
    StatusList *sl;

#ifdef __GRASS__
    sl = (StatusList *) G_malloc(sizeof(StatusList));
#else
    sl = (StatusList *) malloc(sizeof(StatusList));
#endif
    assert(sl);
	
    TreeValue tv;
	
    tv.gradient = SMALLEST_GRADIENT;
    tv.key = 0;
    tv.maxGradient = SMALLEST_GRADIENT;
	
	
    sl->rbt = create_tree(tv);
    return sl;
}


/* ------------------------------------------------------------ */
/*delete a status structure */
void delete_status_structure(StatusList * sl)
{
    assert(sl);
    delete_tree(sl->rbt);
#ifdef __GRASS__
    G_free(sl);
#else
    free(sl);
#endif
    return;
}


/* ------------------------------------------------------------ */
/*delete the statusNode with the given key */
void delete_from_status_struct(StatusList * sl, double dist2vp)
{
    assert(sl);
    delete_from(sl->rbt, dist2vp);
    return;
}




/* ------------------------------------------------------------ */
/*insert the element into the status structure */
void insert_into_status_struct(StatusNode sn, StatusList * sl)
{
    assert(sl);
    TreeValue tv;

    tv.key = sn.dist2vp;
    tv.gradient = sn.gradient;
    tv.maxGradient = SMALLEST_GRADIENT;
    insert_into(sl->rbt, tv);
    return;
}


/* ------------------------------------------------------------ */
/*find the node with max Gradient within the distance (from viewpoint)
   //given */
double find_max_gradient_in_status_struct(StatusList * sl, double dist)
{
    assert(sl);
    /*note: if there is nothing in the status struccture, it means this
       cell is VISIBLE */
    if (is_empty(sl))
	return SMALLEST_GRADIENT;
    /*it is also possible that the status structure is not empty, but
       there are no events with key < dist ---in this case it returns
       SMALLEST_GRADIENT; */
    return find_max_gradient_within_key(sl->rbt, dist);
}

/*returns true is it is empty */
int is_empty(StatusList * sl)
{
    assert(sl);
    return (is_empty(sl->rbt) ||
	    sl->rbt->root->value.maxGradient == SMALLEST_GRADIENT);
}

