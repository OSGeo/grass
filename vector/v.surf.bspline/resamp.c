
/**********************************************************************
 *
 * MODULE:       v.surf.bspline
 *
 * AUTHOR(S):    Roberto Antolin & Gonzalo Moreno
 *               update for grass7 by Markus Metz
 *
 * PURPOSE:      Spline Interpolation
 *
 * COPYRIGHT:    (C) 2006 by Politecnico di Milano -
 *			     Polo Regionale di Como
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **********************************************************************/

#include <grass/config.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bspline.h"

struct Point *P_Read_Raster_Region_masked(SEGMENT *mask_seg,
				       struct Cell_head *Original,
				       struct bound_box output_box,
				       struct bound_box General,
				       int *num_points, int dim_vect,
				       double mean)
{
    int col, row, startcol, endcol, startrow, endrow, nrows, ncols;
    int pippo, npoints;
    double X, Y;
    struct Point *obs;
    char mask_val;

    pippo = dim_vect;
    obs = (struct Point *)G_calloc(pippo, sizeof(struct Point));

    /* Reading points inside output box and inside General box */

    npoints = 0;
    nrows = Original->rows;
    ncols = Original->cols;

    /* original region = output region
     * -> General box is somewhere inside output region */
    if (Original->north > General.N) {
	startrow = (double)((Original->north - General.N) / Original->ns_res - 1);
	if (startrow < 0)
	    startrow = 0;
    }
    else
	startrow = 0;
    if (Original->south < General.S) {
	endrow = (double)((Original->north - General.S) / Original->ns_res + 1);
	if (endrow > nrows)
	    endrow = nrows;
    }
    else
	endrow = nrows;
    if (General.W > Original->west) {
	startcol = (double)((General.W - Original->west) / Original->ew_res - 1);
	if (startcol < 0)
	    startcol = 0;
    }
    else
	startcol = 0;
    if (General.E < Original->east) {
	endcol = (double)((General.E - Original->west) / Original->ew_res + 1);
	if (endcol > ncols)
	    endcol = ncols;
    }
    else
	endcol = ncols;

    for (row = startrow; row < endrow; row++) {
	for (col = startcol; col < endcol; col++) {

	    Segment_get(mask_seg, &mask_val, row, col);
	    if (!mask_val)
		continue;

	    X = Rast_col_to_easting((double)(col) + 0.5, Original);
	    Y = Rast_row_to_northing((double)(row) + 0.5, Original);

	    /* Here, mean is just for asking if obs point is in box */
	    if (Vect_point_in_box(X, Y, mean, &General)) { /* General */
		if (npoints >= pippo) {
		    pippo += dim_vect;
		    obs =
			(struct Point *)G_realloc((void *)obs,
						  (signed int)pippo *
						  sizeof(struct Point));
		}

		/* Storing observation vector */
		obs[npoints].coordX = X;
		obs[npoints].coordY = Y;
		obs[npoints].coordZ = 0;
		npoints++;
	    }
	}
    }

    *num_points = npoints;
    return obs;
}

int P_Sparse_Raster_Points(SEGMENT *out_seg, struct Cell_head *Elaboration,
		struct Cell_head *Original, struct bound_box General, struct bound_box Overlap,
		struct Point *obs, double *param, double pe, double pn,
		double overlap, int nsplx, int nsply, int num_points,
		int bilin, double mean)
{
    int i, row, col;
    double X, Y, interpolation, csi, eta, weight, dval;
    int points_in_box = 0;

    /* Reading points inside output region and inside general box */
    /* all points available here are inside the output box,
     * selected by P_Read_Raster_Region_Nulls(), no check needed */

    for (i = 0; i < num_points; i++) {

	X = obs[i].coordX;
	Y = obs[i].coordY;

	/* X,Y are cell center cordinates, MUST be inside General box */
	row = (int) (floor(Rast_northing_to_row(Y, Original)) + 0.1);
	col = (int) (floor((X - Original->west) / Original->ew_res) + 0.1);

	if (row < 0 || row >= Original->rows) {
	    G_fatal_error("row index out of range");
	    continue;
	}

	if (col < 0 || col >= Original->cols) {
	    G_fatal_error("col index out of range");
	    continue;
	}
	points_in_box++;

	G_debug(3, "P_Sparse_Raster_Points: interpolate point %d...", i);
	if (bilin)
	    interpolation =
		dataInterpolateBilin(X, Y, pe, pn, nsplx,
				     nsply, Elaboration->west,
				     Elaboration->south, param);
	else
	    interpolation =
		dataInterpolateBicubic(X, Y, pe, pn, nsplx,
				       nsply, Elaboration->west,
				       Elaboration->south, param);

	interpolation += mean;

	if (Vect_point_in_box(X, Y, interpolation, &Overlap)) {	/* (5) */
	    dval = interpolation;
	}
	else {
	    Segment_get(out_seg, &dval, row, col);
	    if ((X > Overlap.E) && (X < General.E)) {
		if ((Y > Overlap.N) && (Y < General.N)) {	/* (3) */
		    csi = (General.E - X) / overlap;
		    eta = (General.N - Y) / overlap;
		    weight = csi * eta;
		    interpolation *= weight;
		    dval += interpolation;
		}
		else if ((Y < Overlap.S) && (Y > General.S)) {	/* (1) */
		    csi = (General.E - X) / overlap;
		    eta = (Y - General.S) / overlap;
		    weight = csi * eta;
		    interpolation *= weight;
		    dval = interpolation;
		}
		else if ((Y >= Overlap.S) && (Y <= Overlap.N)) {	/* (1) */
		    weight = (General.E - X ) / overlap;
		    interpolation *= weight;
		    dval = interpolation;
		}
	    }
	    else if ((X < Overlap.W) && (X > General.W)) {
		if ((Y > Overlap.N) && (Y < General.N)) {	/* (4) */
		    csi = (X - General.W) / overlap;
		    eta = (General.N - Y) / overlap;
		    weight = eta * csi;
		    interpolation *= weight;
		    dval += interpolation;
		}
		else if ((Y < Overlap.S) && (Y > General.S)) {	/* (2) */
		    csi = (X - General.W) / overlap;
		    eta = (Y - General.S) / overlap;
		    weight = csi * eta;
		    interpolation *= weight;
		    dval += interpolation;
		}
		else if ((Y >= Overlap.S) && (Y <= Overlap.N)) {	/* (2) */
		    weight = (X - General.W) / overlap;
		    interpolation *= weight;
		    dval += interpolation;
		}
	    }
	    else if ((X >= Overlap.W) && (X <= Overlap.E)) {
		if ((Y > Overlap.N) && (Y < General.N)) {	/* (3) */
		    weight = (General.N - Y) / overlap;
		    interpolation *= weight;
		    dval += interpolation;
		}
		else if ((Y < Overlap.S) && (Y > General.S)) {	/* (1) */
		    weight = (Y - General.S) / overlap;
		    interpolation *= weight;
		    dval = interpolation;
		}
	    }
	} /* end not in overlap */
	Segment_put(out_seg, &dval, row, col);
    }  /* for num_points */

    return 1;
}

/* align elaboration box to source region
 * grow each side */
int align_elaboration_box(struct Cell_head *elaboration, struct Cell_head *original, int type)
{
    int row, col;

    switch (type) {
    case GENERAL_ROW:		/* General case N-S direction */
	/* northern edge */
	row = (int)((original->north - elaboration->north) / original->ns_res);

	if (row < 0)
	    row = 0;

	elaboration->north = original->north - original->ns_res * row;
	
	/* southern edge */
	row = (int)((original->north - elaboration->south) / original->ns_res) + 1;

	if (row > original->rows + 1)
	    row = original->rows + 1;

	elaboration->south = original->north - original->ns_res * row;

	return 1;
    
    case GENERAL_COLUMN:	/* General case E-W direction */

	/* eastern edge */
	col = (int)((elaboration->east - original->west) / original->ew_res) + 1;

	if (col > original->cols + 1)
	    col = original->cols + 1;

	elaboration->east = original->west + original->ew_res * col;
	
	/* western edge */
	col = (int)((elaboration->west - original->west) / original->ew_res);

	if (col < 0)
	    col = 0;

	elaboration->west = original->west + original->ew_res * col;

	return 1;
    }
    return 0;
}


/* align interpolation boxes to destination region
 * return 1 on success, 0 on failure */

int align_interp_boxes(struct bound_box *general, struct bound_box *overlap,
                       struct Cell_head *original, struct bound_box last_general,
		       struct bound_box last_overlap, int type)
{
    int row, col;

    switch (type) {
    case GENERAL_ROW:		/* General case N-S direction */

	/* general box */
	/* grow north */
	general->N = last_overlap.S;

	/* shrink south */
	row = (int)((original->north - general->S) / original->ns_res);

	if (row > original->rows + 1)
	    row = original->rows + 1;

	general->S = original->north - original->ns_res * row;
	
	/* overlap box */
	/* grow north */
	overlap->N = last_general.S;

	/* shrink south */
	row = (int)((original->north - overlap->S) / original->ns_res);

	if (row > original->rows + 1)
	    row = original->rows + 1;

	overlap->S = original->north - original->ns_res * row;

	return 1;

    case GENERAL_COLUMN:	/* General case E-W direction */

	/* general box */
	/* grow west */
	general->W = last_overlap.E;

	/* shrink east */
	col = (int)((general->E - original->west) / original->ew_res);

	if (col > original->cols + 1)
	    col = original->cols + 1;

	general->E = original->west + original->ew_res * col;
	
	/* overlap box */
	/* grow west */
	overlap->W = last_general.E;

	/* shrink east */
	col = (int)((overlap->E - original->west) / original->ew_res);

	if (col > original->cols + 1)
	    col = original->cols + 1;

	overlap->E = original->west + original->ew_res * col;

	return 1;

    case FIRST_ROW:		/* Just started with first row */
	general->N = original->north;
	overlap->N = original->north;

	/* shrink south */
	row = (int)((original->north - general->S) / original->ns_res);

	if (row > original->rows)
	    row = original->rows;

	general->S = original->north - original->ns_res * row;

	row = (int)((original->north - overlap->S) / original->ns_res);

	if (row > original->rows + 1)
	    row = original->rows + 1;

	overlap->S = original->north - original->ns_res * row;

	return 1;

    case LAST_ROW:		/* Reached last row */
	general->S = original->south;
	overlap->S = original->south;

	return 1;

    case FIRST_COLUMN:		/* Just started with first column */
	general->W = original->west;
	overlap->W = original->west;

	/* shrink east */
	col = (int)((general->E - original->west) / original->ew_res);

	if (col > original->cols + 1)
	    col = original->cols + 1;

	general->E = original->west + original->ew_res * col;

	col = (int)((overlap->E - original->west) / original->ew_res);

	if (col > original->cols + 1)
	    col = original->cols + 1;

	overlap->E = original->west + original->ew_res * col;

	return 1;

    case LAST_COLUMN:		/* Reached last column */
	general->E = original->east;
	overlap->E = original->east;

	return 1;
    }

    return 0;
}
