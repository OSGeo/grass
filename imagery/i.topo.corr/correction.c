/* File: correction.c
 *
 *  AUTHOR:    E. Jorge Tizado, Spain 2010
 *
 *  COPYRIGHT: (c) 2007-10 E. Jorge Tizado
 *             This program is free software under the GNU General Public
 *             License (>=v2). Read the file COPYING that comes with GRASS
 *             for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

void eval_tcor(int method, Gfile * out, Gfile * cosi, Gfile * band,
	       double zenith, int do_scale)
{
    int row, col, nrows, ncols;
    void *pref, *pcos;

    double cos_z, cos_i, ref_i, result;
    double n, sx, sxx, sy, sxy, tx, ty;
    double a, m, cka, ckb, kk;

    double imin, imax, omin, omax, factor;    /* for scaling to input */

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    cos_z = cos(D2R * zenith);
    
    imin = omin = DBL_MAX;
    imax = omax = -DBL_MAX;
    factor = 1;

    /* Calculating regression */
    if (method > NON_LAMBERTIAN) {
	n = sx = sxx = sy = sxy = 0.;
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);

	    Rast_get_row(band->fd, band->rast, row, band->type);
	    Rast_get_row(cosi->fd, cosi->rast, row, cosi->type);
	    
	    pref = band->rast;
	    pcos = cosi->rast;

	    for (col = 0; col < ncols; col++) {
		
		cos_i = Rast_get_d_value(pcos, cosi->type);

		if (!Rast_is_null_value(pref, band->type) &&
		    !Rast_is_null_value(pcos, cosi->type)) {
		    ref_i = Rast_get_d_value(pref, band->type);
		    
		    if (imin > ref_i)
			imin = ref_i;
		    if (imax < ref_i)
			imax = ref_i;
		    
		    switch (method) {
		    case MINNAERT:
			if (cos_i > 0. && cos_z > 0. && ref_i > 0.) {
			    n++;
                            /* tx = log(cos_i / cos_z) */
                            /* cos_z is constant then m not changes */
                            tx = log(cos_i);
			    ty = log(ref_i);
			    sx += tx;
			    sxx += tx * tx;
			    sy += ty;
			    sxy += tx * ty;
			}
			break;
		    case C_CORRECT:
			{
			    n++;
			    sx += cos_i;
			    sxx += cos_i * cos_i;
			    sy += ref_i;
			    sxy += cos_i * ref_i;
			}
			break;
		    }
		}
		pref = G_incr_void_ptr(pref, Rast_cell_size(band->type));
		pcos = G_incr_void_ptr(pcos, Rast_cell_size(cosi->type));
	    }
	}
	m = (n == 0.) ? 1. : (n * sxy - sx * sy) / (n * sxx - sx * sx);
	a = (n == 0.) ? 0. : (sy - m * sx) / n;
    }
    /* Calculating Constants */
    switch (method) {
    case MINNAERT:
	cka = ckb = 0.;
	kk = m;
	G_message("Minnaert constant = %lf", kk);
	break;
    case C_CORRECT:
	cka = ckb = a / m; /* Richter changes to m/a */
	kk = 1.;
	G_message("C-factor constant = %lf (a=%.4f; m=%.4f)", cka, a, m);
	break;
    case PERCENT:
	cka = 2. - cos_z;
	ckb = 1.;
	kk = 1.;
	break;
    default:			/* COSINE */
	cka = ckb = 0.;
	kk = 1.;
    }

    if (do_scale) {
	/* Topographic correction, pass 1 */
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);

	    Rast_get_row(band->fd, band->rast, row, band->type);
	    Rast_get_row(cosi->fd, cosi->rast, row, cosi->type);

	    pref = band->rast;
	    pcos = cosi->rast;
	    
	    for (col = 0; col < ncols; col++) {

		cos_i = Rast_get_d_value(pcos, cosi->type);

		if (!Rast_is_null_value(pref, band->type) &&
		    !Rast_is_null_value(pcos, cosi->type)) {

		    ref_i = Rast_get_d_value(pref, band->type);
		    result = (DCELL) (ref_i * pow((cos_z + cka) /
		                      (cos_i + ckb), kk));
		    G_debug(3,
			    "Old val: %f, cka: %f, cos_i: %f, ckb: %f, kk: %f, New val: %f",
			    ref_i, cka, cos_i, ckb, kk, result);

		    if (omin > result)
			omin = result;
		    if (omax < result)
			omax = result;

		}
		pref = G_incr_void_ptr(pref, Rast_cell_size(band->type));
		pcos = G_incr_void_ptr(pcos, Rast_cell_size(cosi->type));
	    }
	}
	G_percent(1, 1, 1);
	factor = (imax - imin) / (omax - omin);
    }
    /* Topographic correction */
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);

	Rast_get_row(band->fd, band->rast, row, band->type);
	Rast_get_row(cosi->fd, cosi->rast, row, cosi->type);

	pref = band->rast;
	pcos = cosi->rast;
	
	Rast_set_null_value(out->rast, ncols, DCELL_TYPE);

	for (col = 0; col < ncols; col++) {

	    cos_i = Rast_get_d_value(pcos, cosi->type);

	    if (!Rast_is_null_value(pref, band->type) &&
		!Rast_is_null_value(pcos, cosi->type)) {

		ref_i = Rast_get_d_value(pref, band->type);
		result = (DCELL) (ref_i * pow((cos_z + cka) /
		                  (cos_i + ckb), kk));

		if (do_scale)
		    result = (result - omin) * factor + imin;

		((DCELL *) out->rast)[col] = result;
		G_debug(3,
			"Old val: %f, cka: %f, cos_i: %f, ckb: %f, kk: %f, New val: %f",
			ref_i, cka, cos_i, ckb, kk,
			((DCELL *) out->rast)[col]);
	    }
	    pref = G_incr_void_ptr(pref, Rast_cell_size(band->type));
	    pcos = G_incr_void_ptr(pcos, Rast_cell_size(cosi->type));
	}
	Rast_put_row(out->fd, out->rast, out->type);
    }
    G_percent(1, 1, 1);
}
