
/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu

 *               Ported to GRASS by William Richard -
 *               wkrichar@bowdoin.edu or willster3021@gmail.com
 *               Markus Metz: surface interpolation
 *
 * Date:         april 2011 
 * 
 * PURPOSE: To calculate the viewshed (the visible cells in the
 * raster) for the given viewpoint (observer) location.  The
 * visibility model is the following: Two points in the raster are
 * considered visible to each other if the cells where they belong are
 * visible to each other.  Two cells are visible to each other if the
 * line-of-sight that connects their centers does not intersect the
 * terrain. The terrain is NOT viewed as a tesselation of flat cells, 
 * i.e. if the line-of-sight does not pass through the cell center, 
 * elevation is determined using bilinear interpolation.
 * The viewshed algorithm is efficient both in
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

extern "C"
{
#include <grass/gis.h>
#include <grass/glocale.h>
}
#include "grass.h"

#include "statusstructure.h"


/*SMALLEST_GRADIENT is defined in rbbst.h */


/* ------------------------------------------------------------ */
/*find the vertical angle in degrees between the viewpoint and the
   point represented by the StatusNode.  Assumes all values (except
   gradient) in sn have been filled. The value returned is in [0,
   180]. A value of 0 is directly below the specified viewing position,
   90 is due horizontal, and 180 is directly above the observer. 
   If doCurv is set we need to consider the curvature of the
   earth */
float get_vertical_angle(Viewpoint vp, StatusNode sn, surface_type elev, int doCurv)
{

    /*determine the difference in elevation, based on the curvature */
    double diffElev;
    diffElev = vp.elev - elev;

    /*calculate and return the angle in degrees */
    assert(fabs(sn.dist2vp) > 0.001);

    /* 0 above, 180 below */
    if (diffElev >= 0.0)
	return (atan(sqrt(sn.dist2vp) / diffElev) * (180 / M_PI));
    else
	return (atan(fabs(diffElev) / sqrt(sn.dist2vp)) * (180 / M_PI) + 90);

    /* 180 above, 0 below */
    if (diffElev >= 0.0)
	return (atan(diffElev / sqrt(sn.dist2vp)) * (180 / M_PI) + 90);
    else
	return (atan(sqrt(sn.dist2vp) / fabs(diffElev)) * (180 / M_PI));
}



/* ------------------------------------------------------------ */
/*return an estimate of the size of active structure */
long long get_active_str_size_bytes(GridHeader * hd)
{

    long long sizeBytes;

    G_verbose_message(_("Estimated size active structure:"));
    G_verbose_message(_(" (key=%d, ptr=%d, total node=%d B)"),
	   (int)sizeof(TreeValue),
	   (int)sizeof(TreeNode *), (int)sizeof(TreeNode));
    sizeBytes = sizeof(TreeNode) * std::max(hd->ncols, hd->nrows);
    G_verbose_message(_(" Total= %lld B"), sizeBytes);
    return sizeBytes;
}


/* ------------------------------------------------------------ */
/*given a StatusNode, fill in its dist2vp and gradient */
void calculate_dist_n_gradient(StatusNode * sn, double elev,
                               Viewpoint * vp, GridHeader hd)
{
    assert(sn && vp);
    /*sqrt is expensive
       //sn->dist2vp = sqrt((float) ( pow(sn->row - vp->row,2.0) + 
       //               pow(sn->col - vp->col,2.0)));
       //sn->gradient = (sn->elev  - vp->elev)/(sn->dist2vp); */
       
    double diffElev = elev - vp->elev;
    
    if (G_projection() == PROJECTION_LL) {
	double dist = G_distance(Rast_col_to_easting(sn->col + 0.5, &(hd.window)),
				 Rast_row_to_northing(sn->row + 0.5, &(hd.window)),
				 Rast_col_to_easting(vp->col + 0.5, &(hd.window)),
				 Rast_row_to_northing(vp->row + 0.5, &(hd.window)));

	sn->dist2vp = dist * dist;
    }
    else {
	double dx = ((double)sn->col - vp->col) * hd.ew_res;
	double dy = ((double)sn->row - vp->row) * hd.ns_res;
	
	sn->dist2vp = (dx * dx) + (dy * dy);
    }

    if (diffElev == 0) {
	sn->gradient[1] = 0;
	return;
    }

    /* PI / 2 above, - PI / 2 below, like r.los */
    sn->gradient[1] = atan(diffElev / sqrt(sn->dist2vp));

    return;

    /* PI above, 0 below. slower than r.los - like */
    if (diffElev >= 0.0)
	sn->gradient[1] = (atan(diffElev / sqrt(sn->dist2vp)) + M_PI / 2);
    else
	sn->gradient[1] = (atan(sqrt(sn->dist2vp) / fabs(diffElev)));

    return;

    /* a little bit faster but not accurate enough */
    sn->gradient[1] = (diffElev * diffElev) / (sn->dist2vp);
    /*maintain sign */
    if (elev < vp->elev)
	sn->gradient[1] = -sn->gradient[1];
	
    return;
}


/* ------------------------------------------------------------ */
/* calculate gradient for ENTERING or EXITING event */
void calculate_event_gradient(StatusNode * sn, int e_idx, 
                    double row, double col, double elev,
		    Viewpoint * vp, GridHeader hd)
{
    assert(sn && vp);
    /*sqrt is expensive
       //sn->dist2vp = sqrt((float) ( pow(sn->row - vp->row,2.0) + 
       //               pow(sn->col - vp->col,2.0)));
       //sn->gradient = (sn->elev  - vp->elev)/(sn->dist2vp); */
       
    double diffElev = elev - vp->elev;
    double dist2vp;

    if (G_projection() == PROJECTION_LL) {
	double dist = G_distance(Rast_col_to_easting(col + 0.5, &(hd.window)),
				 Rast_row_to_northing(row + 0.5, &(hd.window)),
				 Rast_col_to_easting(vp->col + 0.5, &(hd.window)),
				 Rast_row_to_northing(vp->row + 0.5, &(hd.window)));

	dist2vp = dist * dist;
    }
    else {
	double dx = (col - vp->col) * hd.ew_res;
	double dy = (row - vp->row) * hd.ns_res;
	
	dist2vp = (dx * dx) + (dy * dy);
    }

    /* PI / 2 above, - PI / 2 below */
    sn->gradient[e_idx] = atan(diffElev / sqrt(dist2vp));

    return;

    /* PI above, 0 below. slower than r.los - like */
    if (diffElev >= 0.0)
	sn->gradient[e_idx] = (atan(diffElev / sqrt(dist2vp)) + M_PI / 2);
    else
	sn->gradient[e_idx] = (atan(sqrt(dist2vp) / fabs(diffElev)));

    return;

    /* faster but not accurate enough */
    sn->gradient[e_idx] = (diffElev * diffElev) / (dist2vp);
    /*maintain sign */
    if (elev < vp->elev)
	sn->gradient[e_idx] = -sn->gradient[e_idx];

    return;
}


/* ------------------------------------------------------------ */
/*create an empty  status list */
StatusList *create_status_struct()
{
    StatusList *sl;

    sl = (StatusList *) G_malloc(sizeof(StatusList));
    assert(sl);

    TreeValue tv;

    tv.gradient[0] = SMALLEST_GRADIENT;
    tv.gradient[1] = SMALLEST_GRADIENT;
    tv.gradient[2] = SMALLEST_GRADIENT;
    tv.angle[0] = 0;
    tv.angle[1] = 0;
    tv.angle[2] = 0;
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
    G_free(sl);

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
    tv.gradient[0] = sn.gradient[0];
    tv.gradient[1] = sn.gradient[1];
    tv.gradient[2] = sn.gradient[2];
    tv.angle[0] = sn.angle[0];
    tv.angle[1] = sn.angle[1];
    tv.angle[2] = sn.angle[2];
    tv.maxGradient = SMALLEST_GRADIENT;
    insert_into(sl->rbt, tv);

    return;
}


/* ------------------------------------------------------------ */
/*find the node with max Gradient within the distance (from viewpoint)
   //given */
double find_max_gradient_in_status_struct(StatusList * sl, double dist, double angle, double gradient)
{
    assert(sl);
    /*note: if there is nothing in the status struccture, it means this
       cell is VISIBLE */
    if (is_empty(sl))
	return SMALLEST_GRADIENT;
    /*it is also possible that the status structure is not empty, but
       there are no events with key < dist ---in this case it returns
       SMALLEST_GRADIENT; */
    return find_max_gradient_within_key(sl->rbt, dist, angle, gradient);
}

/*returns true if it is empty */
int is_empty(StatusList * sl)
{
    assert(sl);
    return (is_empty(sl->rbt) ||
	    sl->rbt->root->value.maxGradient == SMALLEST_GRADIENT);
}
