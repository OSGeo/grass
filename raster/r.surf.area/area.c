#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>

#include "local_proto.h"

void add_row_area(DCELL * top, DCELL * bottom, double sz, struct Cell_head *w,
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

/* return the cross product v3 = v1 cross v2 */
void v3cross(double v1[3], double v2[3], double v3[3])
{
    v3[X] = (v1[Y] * v2[Z]) - (v1[Z] * v2[Y]);
    v3[Y] = (v1[Z] * v2[X]) - (v1[X] * v2[Z]);
    v3[Z] = (v1[X] * v2[Y]) - (v1[Y] * v2[X]);
}

/* magnitude of vector */
void v3mag(double v1[3], double *mag)
{
    *mag = sqrt(v1[X] * v1[X] + v1[Y] * v1[Y] + v1[Z] * v1[Z]);
}

double conv_value(double value, int units)
{
    if (units == U_UNDEFINED)
	return value;

    return value * G_meters_to_units_factor_sq(units);
}
