
/*-
 * Written by H. Mitasova, I. Kosinovsky, D. Gerdes Fall 1993
 * Copyright 1993, H. Mitasova ,
 * I. Kosinovsky, and D.Gerdes   
 *
 * modified by McCauley in August 1995
 * modified by Mitasova in August 1995  
 * modified by Brown in June 1999 - added elatt & smatt 
 *
 */

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/interpf.h>

void IL_init_params_2d(
    /* initialize parameters */
    struct interp_params *params,
    FILE * inp,			/* input stream */
    int elatt,			/* which fp att in sites file? 1 = first */
    int smatt,			/* which fp att in sites file to use for 
				 * smoothing? (if zero use sm) 1 = first */
    double zm,			/* multiplier for z-values */
    int k1,			/* min number of points per segment for
				 * interpolation */
    int k2,			/* max number of points per segment */
    char *msk,			/* name of mask */
    int rows, int cols, 	/* number of rows and columns */
    DCELL * ar1, DCELL * ar2,
    DCELL * ar3, DCELL * ar4,
    DCELL * ar5, DCELL * ar6,	/* arrays for interpolated values */
    double tension,		/* tension */
    int k3,			/* max num. of points for interp. */
    int sc1, int sc2, int sc3,	/* multipliers for interp. values */
    double sm,			/* smoothing */
    char *f1, char *f2,
    char *f3, char *f4,
    char *f5, char *f6,		/* output files */
    double dm,			/* min distance between points */
    double x_or, double y_or,	/* origin */
    int der,			/* 1 if compute partial derivs */
    double tet,			/* anisotropy angle, 0=East,counter-clockwise */
    double scl,			/* anisotropy scaling factor */
    FILE * t1, FILE * t2,
    FILE * t3, FILE * t4,
    FILE * t5, FILE * t6,	/* temp files for writing interp. values */
    FILE * dev,			/* pointer to deviations file */
    struct TimeStamp *ts,
    int c,			/* cross validation */
    const char *wheresql	/* SQL WHERE */
    )
{
    params->fdinp = inp;
    params->elatt = elatt;
    params->smatt = smatt;
    params->zmult = zm;
    params->kmin = k1;
    params->kmax = k2;
    params->maskmap = msk;
    params->nsizr = rows;
    params->nsizc = cols;
    params->az = ar1;
    params->adx = ar2;
    params->ady = ar3;
    params->adxx = ar4;
    params->adyy = ar5;
    params->adxy = ar6;
    params->fi = tension;
    params->KMAX2 = k3;
    params->scik1 = sc1;
    params->scik2 = sc2;
    params->scik3 = sc3;
    params->rsm = sm;
    params->elev = f1;
    params->slope = f2;
    params->aspect = f3;
    params->pcurv = f4;
    params->tcurv = f5;
    params->mcurv = f6;
    params->dmin = dm;
    params->x_orig = x_or;
    params->y_orig = y_or;
    params->deriv = der;
    params->theta = tet;
    params->scalex = scl;
    params->Tmp_fd_z = t1;
    params->Tmp_fd_dx = t2;
    params->Tmp_fd_dy = t3;
    params->Tmp_fd_xx = t4;
    params->Tmp_fd_yy = t5;
    params->Tmp_fd_xy = t6;
    params->fddevi = dev;
    params->ts = ts;
    params->cv = c;
    params->wheresql = wheresql;
}

void IL_init_func_2d(struct interp_params *params,
		     grid_calc_fn *grid_f,	/* calculates grid for given segm */
		     matrix_create_fn *matr_f,	/* creates matrix for a given segm */
		     check_points_fn *point_f,	/* checks interp. func. at points */
		     secpar_fn *secp_f,		/* calculates aspect,slope,curv. */
		     interp_fn *interp_f,	/* radial  basis function */
		     interpder_fn *interpder_f,	/* derivatives of radial basis func. */
		     wr_temp_fn *temp_f		/* writes temp files */
    )
/* initialize functions */
{
    params->grid_calc = grid_f;
    params->matrix_create = matr_f;
    params->check_points = point_f;
    params->secpar = secp_f;
    params->interp = interp_f;
    params->interpder = interpder_f;
    params->wr_temp = temp_f;

}


