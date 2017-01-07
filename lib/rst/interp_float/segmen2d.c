/*!
 * \file segmen2d.c
 *
 * \author H. Mitasova, I. Kosinovsky, D. Gerdes
 *
 * \copyright
 * (C) 1993 by Helena Mitasova and the GRASS Development Team
 *
 * \copyright
 * This program is free software under the
 * GNU General Public License (>=v2).
 * Read the file COPYING that comes with GRASS
 * for details.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/interpf.h>
#include <grass/gmath.h>


/*!
 * Interpolate recursively a tree of segments
 *
 *  Recursively processes each segment in a tree by:
 *  - finding points from neighbouring segments so that the total number of
 *    points is between KMIN and KMAX2 by calling tree function MT_get_region().
 *  - creating and solving the system of linear equations using these points
 *    and interp() by calling matrix_create() and G_ludcmp().
 *  - checking the interpolating function values at points by calling
 *    check_points().
 *  - computing grid for this segment using points and interp() by calling
 *    grid_calc().
 * 
 * \todo
 * Isn't this in fact the updated version of the function (IL_interp_segments_new_2d)?
 * The function IL_interp_segments_new_2d has the following, better behavior:
 * The difference between this function and IL_interp_segments_2d() is making
 * sure that additional points are taken from all directions, i.e. it finds
 * equal number of points from neighboring segments in each of 8 neighborhoods.
 */
int IL_interp_segments_2d(struct interp_params *params,
                          struct tree_info *info,  /*!< info for the quad tree */
                          struct multtree *tree,  /*!< current leaf of the quad tree */
                          struct BM *bitmask,  /*!< bitmask */
                          double zmin, double zmax,  /*!< min and max input z-values */
                          double *zminac, double *zmaxac,  /*!< min and max interp. z-values */
                          double *gmin, double *gmax,  /*!< min and max inperp. slope val. */
                          double *c1min, double *c1max,  /*!< min and max interp. curv. val. */
                          double *c2min, double *c2max,  /*!< min and max interp. curv. val. */
                          double *ertot,  /*!< total interplating func. error */
                          int totsegm,  /*!< total number of segments */
                          off_t offset1,  /*!< offset for temp file writing */
                          double dnorm)
{
    double xmn, xmx, ymn, ymx, distx, disty, distxp, distyp, temp1, temp2;
    int i, npt, nptprev, MAXENC;
    struct quaddata *data;
    static int cursegm = 0;
    static double *b = NULL;
    static int *indx = NULL;
    static double **matrix = NULL;
    double ew_res, ns_res;
    static int first_time = 1;
    static double smseg;
    int MINPTS;
    double pr;
    struct triple *point;
    struct triple skip_point;
    int m_skip, skip_index, j, k, segtest;
    double xx, yy, zz;

    /* find the size of the smallest segment once */
    if (first_time) {
	smseg = smallest_segment(info->root, 4);
	first_time = 0;
    }
    ns_res = (((struct quaddata *)(info->root->data))->ymax -
	      ((struct quaddata *)(info->root->data))->y_orig) /
	params->nsizr;
    ew_res =
	(((struct quaddata *)(info->root->data))->xmax -
	 ((struct quaddata *)(info->root->data))->x_orig) / params->nsizc;

    if (tree == NULL)
	return -1;
    if (tree->data == NULL)
	return -1;
    if (((struct quaddata *)(tree->data))->points == NULL) {
	for (i = 0; i < 4; i++) {
	    IL_interp_segments_2d(params, info, tree->leafs[i],
				  bitmask, zmin, zmax, zminac, zmaxac, gmin,
				  gmax, c1min, c1max, c2min, c2max, ertot,
				  totsegm, offset1, dnorm);
	}
	return 1;
    }
    else {
	distx = (((struct quaddata *)(tree->data))->n_cols * ew_res) * 0.1;
	disty = (((struct quaddata *)(tree->data))->n_rows * ns_res) * 0.1;
	distxp = 0;
	distyp = 0;
	xmn = ((struct quaddata *)(tree->data))->x_orig;
	xmx = ((struct quaddata *)(tree->data))->xmax;
	ymn = ((struct quaddata *)(tree->data))->y_orig;
	ymx = ((struct quaddata *)(tree->data))->ymax;
	i = 0;
	MAXENC = 0;
	/* data is a window with zero points; some fields don't make sense in this case
	   so they are zero (like resolution,dimensions */
	/* CHANGE */
	/* Calcutaing kmin for surrent segment (depends on the size) */

/*****if (smseg <= 0.00001) MINPTS=params->kmin; else {} ***/
	pr = pow(2., (xmx - xmn) / smseg - 1.);
	MINPTS =
	    params->kmin * (pr / (1 + params->kmin * pr / params->KMAX2));
	/* fprintf(stderr,"MINPTS=%d, KMIN=%d, KMAX=%d, pr=%lf, smseg=%lf, DX=%lf \n", MINPTS,params->kmin,params->KMAX2,pr,smseg,xmx-xmn); */

	data =
	    (struct quaddata *)quad_data_new(xmn - distx, ymn - disty,
					     xmx + distx, ymx + disty, 0, 0,
					     0, params->KMAX2);
	npt = MT_region_data(info, info->root, data, params->KMAX2, 4);

	while ((npt < MINPTS) || (npt > params->KMAX2)) {
	    if (i >= 70) {
		G_warning(_("Taking too long to find points for interpolation - "
			    "please change the region to area where your points are. "
			    "Continuing calculations..."));
		break;
	    }
	    i++;
	    if (npt > params->KMAX2)
		/* decrease window */
	    {
		MAXENC = 1;
		nptprev = npt;
		temp1 = distxp;
		distxp = distx;
		distx = distxp - fabs(distx - temp1) * 0.5;
		temp2 = distyp;
		distyp = disty;
		disty = distyp - fabs(disty - temp2) * 0.5;
		/* decrease by 50% of a previous change in window */
	    }
	    else {
		nptprev = npt;
		temp1 = distyp;
		distyp = disty;
		temp2 = distxp;
		distxp = distx;
		if (MAXENC) {
		    disty = fabs(disty - temp1) * 0.5 + distyp;
		    distx = fabs(distx - temp2) * 0.5 + distxp;
		}
		else {
		    distx += distx;
		    disty += disty;
		}
		/* decrease by 50% of extra distance */
	    }
	    data->x_orig = xmn - distx;	/* update window */
	    data->y_orig = ymn - disty;
	    data->xmax = xmx + distx;
	    data->ymax = ymx + disty;
	    data->n_points = 0;
	    npt = MT_region_data(info, info->root, data, params->KMAX2, 4);
	}
	
	if (totsegm != 0) {
	    G_percent(cursegm, totsegm, 1);
	}
	data->n_rows = ((struct quaddata *)(tree->data))->n_rows;
	data->n_cols = ((struct quaddata *)(tree->data))->n_cols;

	/* for printing out overlapping segments */
	((struct quaddata *)(tree->data))->x_orig = xmn - distx;
	((struct quaddata *)(tree->data))->y_orig = ymn - disty;
	((struct quaddata *)(tree->data))->xmax = xmx + distx;
	((struct quaddata *)(tree->data))->ymax = ymx + disty;

	data->x_orig = xmn;
	data->y_orig = ymn;
	data->xmax = xmx;
	data->ymax = ymx;

	if (!matrix) {
	    if (!
		(matrix =
		 G_alloc_matrix(params->KMAX2 + 1, params->KMAX2 + 1))) {
		G_warning(_("Out of memory"));
		return -1;
	    }
	}
	if (!indx) {
	    if (!(indx = G_alloc_ivector(params->KMAX2 + 1))) {
		G_warning(_("Out of memory"));
		return -1;
	    }
	}
	if (!b) {
	    if (!(b = G_alloc_vector(params->KMAX2 + 3))) {
		G_warning(_("Out of memory"));
		return -1;
	    }
	}
	/* allocate memory for CV points only if cv is performed */
	if (params->cv) {
	    if (!
		(point =
		 (struct triple *)G_malloc(sizeof(struct triple) *
					   data->n_points))) {
		G_warning(_("Out of memory"));
		return -1;
	    }
	}

	/*normalize the data so that the side of average segment is about 1m */
	/* put data_points into point only if CV is performed */

	for (i = 0; i < data->n_points; i++) {
	    data->points[i].x = (data->points[i].x - data->x_orig) / dnorm;
	    data->points[i].y = (data->points[i].y - data->y_orig) / dnorm;
	    if (params->cv) {
		point[i].x = data->points[i].x;	/*cv stuff */
		point[i].y = data->points[i].y;	/*cv stuff */
		point[i].z = data->points[i].z;	/*cv stuff */
	    }

	    /* commented out by Helena january 1997 as this is not necessary
	       although it may be useful to put normalization of z back? 
	       data->points[i].z = data->points[i].z / dnorm;
	       this made smoothing self-adjusting  based on dnorm
	       if (params->rsm < 0.) data->points[i].sm = data->points[i].sm / dnorm;
	     */
	}

	/* cv stuff */
	if (params->cv)
	    m_skip = data->n_points;
	else
	    m_skip = 1;

	/* remove after cleanup - this is just for testing */
	skip_point.x = 0.;
	skip_point.y = 0.;
	skip_point.z = 0.;


	/*** TODO: parallelize this loop instead of the LU solver! ***/
	for (skip_index = 0; skip_index < m_skip; skip_index++) {
	    if (params->cv) {
		segtest = 0;
		j = 0;
		xx = point[skip_index].x * dnorm + data->x_orig +
		    params->x_orig;
		yy = point[skip_index].y * dnorm + data->y_orig +
		    params->y_orig;
		zz = point[skip_index].z;
		if (xx >= data->x_orig + params->x_orig &&
		    xx <= data->xmax + params->x_orig &&
		    yy >= data->y_orig + params->y_orig &&
		    yy <= data->ymax + params->y_orig) {
		    segtest = 1;
		    skip_point.x = point[skip_index].x;
		    skip_point.y = point[skip_index].y;
		    skip_point.z = point[skip_index].z;
		    for (k = 0; k < m_skip; k++) {
			if (k != skip_index && params->cv) {
			    data->points[j].x = point[k].x;
			    data->points[j].y = point[k].y;
			    data->points[j].z = point[k].z;
			    j++;
			}
		    }
		}		/* segment area test */
	    }
	    if (!params->cv) {
		if (params->
		    matrix_create(params, data->points, data->n_points,
				  matrix, indx) < 0)
		    return -1;
	    }
	    else if (segtest == 1) {
		if (params->
		    matrix_create(params, data->points, data->n_points - 1,
				  matrix, indx) < 0)
		    return -1;
	    }
	    if (!params->cv) {
		for (i = 0; i < data->n_points; i++)
		    b[i + 1] = data->points[i].z;
		b[0] = 0.;
		G_lubksb(matrix, data->n_points + 1, indx, b);
	/* put here condition to skip error if not needed */
		params->check_points(params, data, b, ertot, zmin, dnorm,
				     skip_point);
	    }
	    else if (segtest == 1) {
		for (i = 0; i < data->n_points - 1; i++)
		    b[i + 1] = data->points[i].z;
		b[0] = 0.;
		G_lubksb(matrix, data->n_points, indx, b);
		params->check_points(params, data, b, ertot, zmin, dnorm,
				     skip_point);
	    }
	}			/*end of cv loop */

	if (!params->cv)
	    if ((params->Tmp_fd_z != NULL) || (params->Tmp_fd_dx != NULL) ||
		(params->Tmp_fd_dy != NULL) || (params->Tmp_fd_xx != NULL) ||
		(params->Tmp_fd_yy != NULL) || (params->Tmp_fd_xy != NULL)) {

		if (params->grid_calc(params, data, bitmask,
				      zmin, zmax, zminac, zmaxac, gmin, gmax,
				      c1min, c1max, c2min, c2max, ertot, b,
				      offset1, dnorm) < 0)
		    return -1;
	    }

	/* show after to catch 100% */
	cursegm++;
	if (totsegm < cursegm)
	    G_debug(1, "%d %d", totsegm, cursegm);
	
	if (totsegm != 0) {
	    G_percent(cursegm, totsegm, 1);
	}
	/* 
	   G_free_matrix(matrix);
	   G_free_ivector(indx);
	   G_free_vector(b);
	 */
	G_free(data->points);
	G_free(data);
    }
    return 1;
}

double smallest_segment(struct multtree *tree, int n_leafs)
{
    static int first_time = 1;
    int ii;
    static double minside;
    double side;

    if (tree == NULL)
	return 0;
    if (tree->data == NULL)
	return 0;
    if (tree->leafs != NULL) {
	for (ii = 0; ii < n_leafs; ii++) {
	    side = smallest_segment(tree->leafs[ii], n_leafs);
	    if (first_time) {
		minside = side;
		first_time = 0;
	    }
	    if (side < minside)
		minside = side;
	}
    }
    else {
	side = ((struct quaddata *)(tree->data))->xmax -
	    ((struct quaddata *)(tree->data))->x_orig;
	return side;
    }
    
    return minside;
}
