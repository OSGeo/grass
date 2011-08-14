/* main.c - r.surf.area */

/* Copyright Notice
 * ---------------- 
 * Written by Bill Brown, USACERL December 21, 1994 
 * Copyright 1994, Bill Brown, USACERL
 * brown@gis.uiuc.edu
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


/* Calculates area of regular 3D triangulated points 
 * (centers of cells) in current region by
 * adding areas of triangles.  Therefore, area of a flat surface 
 * will be reported as (rows + cols -1)*(area of cell) less than area of 
 * flat region due to a half row and half column missing around the 
 * perimeter.
 * NOTE:  This calculation is heavily dependent on data resolution
 *     (think of it as a fractal shoreline problem, the more resolution
 *     the more detail, the more area, etc).  This program uses the
 *     CURRENT GRASS REGION, not the resolution of the map.
 * This version actually calculates area twice for each triangle pair,
 * keeping a running minimum and maximum area depending on the
 * direction of the diagonal used.
 * Reported totals are: 
 *      1) "plan" area within calculation region (rows-1 * cols-1 * cellarea)
 *      2) avg of min & max calculated 3d triangle area within this region
 *      3) "plan" area within current GRASS region (rows * cols * cellarea)
 *      4) scaling of calculated area to current GRASS region 
 */

/* Modified by Eric G. Miller to work with FP rasters and to handle
 * NULL value cells.  I'm not too sure how bad the surface area
 * calculation will get if there are alot of NULL cells mixed around.
 * Added function prototypes and removed a couple unneccessary typedefs
 * Added the add_null_area() function.
 * 2000-10-17
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>


#define X 0
#define Y 1
#define Z 2


void add_row_area(DCELL *, DCELL *, double, struct Cell_head *,
		  double *, double *);
void v3cross(double v1[3], double v2[3], double v3[3]);
void v3mag(double v1[3], double *mag);
void add_null_area(DCELL *, struct Cell_head *, double *);


int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *surf, *vscale;
    DCELL *cell_buf[2];
    int row;
    struct Cell_head w;
    int cellfile = -1;
    double minarea, maxarea, sz, nullarea;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("statistics"));
    module->description = _("Surface area estimation for rasters.");

    surf = G_define_option();
    surf->key = "input";
    surf->type = TYPE_STRING;
    surf->required = YES;
    surf->multiple = NO;
    surf->gisprompt = "old,cell,Raster";
    surf->description = _("Raster file for surface");

    vscale = G_define_option();
    vscale->key = "vscale";
    vscale->type = TYPE_DOUBLE;
    vscale->required = NO;
    vscale->multiple = NO;
    vscale->description = _("Vertical scale");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (vscale->answer)
	sz = atof(vscale->answer);
    else
	sz = 1.0;

    G_get_set_window(&w);

    /* open raster map for reading */
    cellfile = Rast_open_old(surf->answer, "");

    cell_buf[0] = (DCELL *) G_malloc(w.cols * Rast_cell_size(DCELL_TYPE));
    cell_buf[1] = (DCELL *) G_malloc(w.cols * Rast_cell_size(DCELL_TYPE));

    fprintf(stdout, "\n");
    {
	DCELL *top, *bottom;

	minarea = maxarea = nullarea = 0.0;
	for (row = 0; row < w.rows - 1; row++) {
	    if (!row) {
		Rast_get_row(cellfile, cell_buf[1], 0, DCELL_TYPE);
		top = cell_buf[1];
	    }
	    Rast_get_row(cellfile, cell_buf[row % 2], row + 1,
			     DCELL_TYPE);
	    bottom = cell_buf[row % 2];
	    add_row_area(top, bottom, sz, &w, &minarea, &maxarea);
	    add_null_area(top, &w, &nullarea);
	    top = bottom;
	    G_percent(row, w.rows, 10);
	}
	/* Get last null row area */
	add_null_area(top, &w, &nullarea);
    }


    G_free(cell_buf[0]);
    G_free(cell_buf[1]);
    Rast_close(cellfile);

    {				/* report */
	double reg_area, flat_area, estavg;

	flat_area = (w.cols - 1) * (w.rows - 1) * w.ns_res * w.ew_res;
	reg_area = w.cols * w.rows * w.ns_res * w.ew_res;
	estavg = (minarea + maxarea) / 2.0;

	fprintf(stdout, "Null value area ignored in calculation %e\n",
		nullarea);
	fprintf(stdout, "Plan area used in calculation: %e\n", flat_area);
	fprintf(stdout,
		"Surface Area Calculation(low, high, avg):\n\t%e %e %e\n",
		minarea, maxarea, estavg);

	fprintf(stdout, "Current Region plan area: %e\n", reg_area);
	fprintf(stdout, "Estimated Region Surface Area: %e\n",
		reg_area * estavg / flat_area);
	fprintf(stdout, "\nDone.\n");
    }

    return (0);			/* Zero means success */
}

/************************************************************************/

void
add_row_area(DCELL * top, DCELL * bottom, double sz, struct Cell_head *w,
	     double *low, double *high)
{
    double guess1, guess2, mag, tedge1[3], tedge2[3], crossp[3];
    int col;

    for (col = 0; col < w->cols - 1; col++) {

	/* 
	   For each cell**, we triangulate the four corners in
	   two different ways, 1) UppperLeft to LowerRight diagonal
	   and 2) LowerLeft to UpperRight diagonal.  Then we add the 
	   smaller of the two areas to "low" and the greater of
	   the two areas to "high". 

	   ** here, the "cell" is actually the quadrangle formed by
	   the center point of four cells, since these are the 
	   known elevation points.
	 */

	/* If NAN go to next or we get NAN for everything */
	if (Rast_is_d_null_value(&(bottom[col + 1])) ||
	    Rast_is_d_null_value(&(top[col])) ||
	    Rast_is_d_null_value(&(top[col + 1])) ||
	    Rast_is_d_null_value(&(bottom[col]))
	    )
	    continue;

	/* guess1 --- ul to lr diag */
	{
	    tedge1[X] = w->ew_res;
	    tedge1[Y] = -w->ns_res;
	    tedge1[Z] = sz * (bottom[col + 1] - top[col]);

	    /* upper */
	    tedge2[X] = 0.0;
	    tedge2[Y] = w->ns_res;
	    tedge2[Z] = sz * (top[col + 1] - bottom[col + 1]);

	    v3cross(tedge1, tedge2, crossp);
	    v3mag(crossp, &mag);
	    guess1 = .5 * mag;

	    /* lower */
	    tedge2[X] = -w->ew_res;
	    tedge2[Y] = 0.0;
	    tedge2[Z] = sz * (bottom[col] - bottom[col + 1]);

	    v3cross(tedge1, tedge2, crossp);
	    v3mag(crossp, &mag);
	    guess1 += .5 * mag;
	}

	/* guess2 --- ll to ur diag */
	{
	    tedge1[X] = w->ew_res;
	    tedge1[Y] = w->ns_res;
	    tedge1[Z] = sz * (top[col + 1] - bottom[col]);

	    /* upper */
	    tedge2[X] = -w->ew_res;
	    tedge2[Y] = 0.0;
	    tedge2[Z] = sz * (top[col + 1] - top[col + 1]);

	    v3cross(tedge1, tedge2, crossp);
	    v3mag(crossp, &mag);
	    guess2 = .5 * mag;

	    /* lower */
	    tedge2[X] = 0.0;
	    tedge2[Y] = -w->ns_res;
	    tedge2[Z] = sz * (bottom[col + 1] - top[col + 1]);

	    v3cross(tedge1, tedge2, crossp);
	    v3mag(crossp, &mag);
	    guess2 += .5 * mag;
	}
	*low += (guess1 < guess2) ? guess1 : guess2;
	*high += (guess1 < guess2) ? guess2 : guess1;

    }				/* ea col */

}

/************************************************************************/
/* calculate the running area of null data cells */
void add_null_area(DCELL * rast, struct Cell_head *region, double *area)
{
    int col;

    for (col = 0; col < region->cols; col++) {
	if (Rast_is_d_null_value(&(rast[col]))) {
	    *area += region->ew_res * region->ns_res;
	}
    }
}

/************************************************************************/
/* return the cross product v3 = v1 cross v2 */
void v3cross(double v1[3], double v2[3], double v3[3])
{
    v3[X] = (v1[Y] * v2[Z]) - (v1[Z] * v2[Y]);
    v3[Y] = (v1[Z] * v2[X]) - (v1[X] * v2[Z]);
    v3[Z] = (v1[X] * v2[Y]) - (v1[Y] * v2[X]);
}

/************************************************************************/
/* magnitude of vector */
void v3mag(double v1[3], double *mag)
{
    *mag = sqrt(v1[X] * v1[X] + v1[Y] * v1[Y] + v1[Z] * v1[Z]);
}

/************************************************************************/
/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
