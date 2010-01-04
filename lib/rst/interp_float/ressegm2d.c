
/*-
 * Written by H. Mitasova, I. Kosinovsky, D. Gerdes Summer 1993
 * University of Illinois
 * US Army Construction Engineering Research Lab  
 * Copyright 1993, H. Mitasova (University of Illinois),
 * I. Kosinovsky, (USA-CERL), and D.Gerdes (USA-CERL)   
 *
 * modified by McCauley in August 1995
 * modified by Mitasova in August 1995  
 *
 * bug fixes by Jaro Hofierka in February 1999:
 *  line: 175,348 (*dnorm)
 *        177,350  (points[m1].sm)
 *         457,461 (})
 *
 * modified by Mitasova November 1999 (option for dnorm ind. tension)
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/interpf.h>
#include <grass/gmath.h>

static int input_data(struct interp_params *,
		      int, int, struct fcell_triple *, int, int, int, int,
		      double, double, double);
static int write_zeros(struct interp_params *, struct quaddata *, off_t);

int IL_resample_interp_segments_2d(struct interp_params *params, struct BM *bitmask,	/* bitmask */
				   double zmin, double zmax,	/* min and max input z-values */
				   double *zminac, double *zmaxac,	/* min and max interp. z-values */
				   double *gmin, double *gmax,	/* min and max inperp. slope val. */
				   double *c1min, double *c1max, double *c2min, double *c2max,	/* min and max interp. curv. val. */
				   double *ertot,	/* total interplating func. error */
				   off_t offset1,	/* offset for temp file writing */
				   double *dnorm,
				   int overlap,
				   int inp_rows,
				   int inp_cols,
				   int fdsmooth,
				   int fdinp,
				   double ns_res,
				   double ew_res,
				   double inp_ns_res,
				   double inp_ew_res, int dtens)
{

    int i, j, k, l, m, m1, i1;	/* loop coounters */
    int cursegm = 0;
    int new_comp = 0;
    int n_rows, n_cols, inp_r, inp_c;
    double x_or, y_or, xm, ym;
    static int first = 1, new_first = 1;
    double **matrix = NULL, **new_matrix = NULL, *b = NULL;
    int *indx = NULL, *new_indx = NULL;
    static struct fcell_triple *in_points = NULL;	/* input points */
    int inp_check_rows, inp_check_cols,	/* total input rows/cols */
      out_check_rows, out_check_cols;	/* total output rows/cols */
    int first_row, last_row;	/* first and last input row of segment */
    int first_col, last_col;	/* first and last input col of segment */
    int num, prev;
    int div;			/* number of divides */
    int rem_out_row, rem_out_col;	/* output rows/cols remainders */
    int inp_seg_r, inp_seg_c,	/* # of input rows/cols in segment */
      out_seg_r, out_seg_c;	/* # of output rows/cols in segment */
    int ngstc, nszc		/* first and last output col of the
				 * segment */
     , ngstr, nszr;		/* first and last output row of the
				 * segment */
    int index;			/* index for input data */
    int c, r;
    int overlap1;
    int p_size;
    struct quaddata *data;
    double xmax, xmin, ymax, ymin;
    int totsegm;		/* total number of segments */
    int total_points = 0;
    struct triple triple;	/* contains garbage */


    xmin = params->x_orig;
    ymin = params->y_orig;
    xmax = xmin + ew_res * params->nsizc;
    ymax = ymin + ns_res * params->nsizr;
    prev = inp_rows * inp_cols;
    if (prev <= params->kmax)
	div = 1;		/* no segmentation */

    else {			/* find the number of divides */
	for (i = 2;; i++) {
	    c = inp_cols / i;
	    r = inp_rows / i;
	    num = c * r;
	    if (num < params->kmin) {
		if (((params->kmin - num) > (prev + 1 - params->kmax)) &&
		    (prev + 1 < params->KMAX2)) {
		    div = i - 1;
		    break;
		}
		else {
		    div = i;
		    break;
		}
	    }
	    if ((num > params->kmin) && (num + 1 < params->kmax)) {
		div = i;
		break;
	    }
	    prev = num;
	}
    }
    out_seg_r = params->nsizr / div;	/* output rows per segment */
    out_seg_c = params->nsizc / div;	/* output cols per segment */
    inp_seg_r = inp_rows / div;	/* input rows per segment */
    inp_seg_c = inp_cols / div;	/* input rows per segment */
    rem_out_col = params->nsizc % div;
    rem_out_row = params->nsizr % div;
    overlap1 = min1(overlap, inp_seg_c - 1);
    overlap1 = min1(overlap1, inp_seg_r - 1);
    out_check_rows = 0;
    out_check_cols = 0;
    inp_check_rows = 0;
    inp_check_cols = 0;

    if (div == 1) {
	p_size = inp_seg_c * inp_seg_r;
    }
    else {
	p_size = (overlap1 * 2 + inp_seg_c) * (overlap1 * 2 + inp_seg_r);
    }
    if (!in_points) {
	if (!
	    (in_points =
	     (struct fcell_triple *)G_malloc(sizeof(struct fcell_triple) *
					     p_size * div))) {
	    fprintf(stderr, "Cannot allocate memory for in_points\n");
	    return -1;
	}
    }

    *dnorm =
	sqrt(((xmax - xmin) * (ymax -
			       ymin) * p_size) / (inp_rows * inp_cols));

    if (dtens) {
	params->fi = params->fi * (*dnorm) / 1000.;
	fprintf(stderr, "dnorm = %f, rescaled tension = %f\n", *dnorm,
		params->fi);
    }

    if (div == 1) {		/* no segmentation */
	totsegm = 1;
	cursegm = 1;

	input_data(params, 1, inp_rows, in_points, fdsmooth, fdinp, inp_rows,
		   inp_cols, zmin, inp_ns_res, inp_ew_res);

	x_or = 0.;
	y_or = 0.;
	xm = params->nsizc * ew_res;
	ym = params->nsizr * ns_res;

	data = (struct quaddata *)quad_data_new(x_or, y_or, xm, ym,
						params->nsizr, params->nsizc,
						0, params->KMAX2);
	m1 = 0;
	for (k = 1; k <= p_size; k++) {
	    if (!Rast_is_f_null_value(&(in_points[k - 1].z))) {
		data->points[m1].x = in_points[k - 1].x / (*dnorm);
		data->points[m1].y = in_points[k - 1].y / (*dnorm);
		/*        data->points[m1].z = (double) (in_points[k - 1].z) / (*dnorm); */
		data->points[m1].z = (double)(in_points[k - 1].z);
		data->points[m1].sm = in_points[k - 1].smooth;
		m1++;
	    }
	}
	data->n_points = m1;
	total_points = m1;
	if (!(indx = G_alloc_ivector(params->KMAX2 + 1))) {
	    fprintf(stderr, "Cannot allocate memory for indx\n");
	    return -1;
	}
	if (!(matrix = G_alloc_matrix(params->KMAX2 + 1, params->KMAX2 + 1))) {
	    fprintf(stderr, "Cannot allocate memory for matrix\n");
	    return -1;
	}
	if (!(b = G_alloc_vector(params->KMAX2 + 2))) {
	    fprintf(stderr, "Cannot allocate memory for b\n");
	    return -1;
	}

	if (params->matrix_create(params, data->points, m1, matrix, indx) < 0)
	    return -1;
	for (i = 0; i < m1; i++) {
	    b[i + 1] = data->points[i].z;
	}
	b[0] = 0.;
	G_lubksb(matrix, m1 + 1, indx, b);

	params->check_points(params, data, b, ertot, zmin, *dnorm, triple);

	if (params->grid_calc(params, data, bitmask,
			      zmin, zmax, zminac, zmaxac, gmin, gmax,
			      c1min, c1max, c2min, c2max, ertot, b, offset1,
			      *dnorm) < 0) {
	    fprintf(stderr, "interpolation failed\n");
	    return -1;
	}
	else {
	    if (totsegm != 0) {
		G_percent(cursegm, totsegm, 1);
	    }
	    /*
	     * if (b) G_free_vector(b); if (matrix) G_free_matrix(matrix); if
	     * (indx) G_free_ivector(indx);
	     */
	    fprintf(stderr, "dnorm in ressegm after grid before out= %f \n",
		    *dnorm);
	    return total_points;
	}
    }

    out_seg_r = params->nsizr / div;	/* output rows per segment */
    out_seg_c = params->nsizc / div;	/* output cols per segment */
    inp_seg_r = inp_rows / div;	/* input rows per segment */
    inp_seg_c = inp_cols / div;	/* input rows per segment */
    rem_out_col = params->nsizc % div;
    rem_out_row = params->nsizr % div;
    overlap1 = min1(overlap, inp_seg_c - 1);
    overlap1 = min1(overlap1, inp_seg_r - 1);
    out_check_rows = 0;
    out_check_cols = 0;
    inp_check_rows = 0;
    inp_check_cols = 0;

    totsegm = div * div;

    /* set up a segment */
    for (i = 1; i <= div; i++) {	/* input and output rows */
	if (i <= div - rem_out_row)
	    n_rows = out_seg_r;
	else
	    n_rows = out_seg_r + 1;
	inp_r = inp_seg_r;
	out_check_cols = 0;
	inp_check_cols = 0;
	ngstr = out_check_rows + 1;	/* first output row of the segment */
	nszr = ngstr + n_rows - 1;	/* last output row of the segment */
	y_or = (ngstr - 1) * ns_res;	/* y origin of the segment */
	/*
	 * Calculating input starting and ending rows and columns of this
	 * segment
	 */
	first_row = (int)(y_or / inp_ns_res) + 1;
	if (first_row > overlap1) {
	    first_row -= overlap1;	/* middle */
	    last_row = first_row + inp_seg_r + overlap1 * 2 - 1;
	    if (last_row > inp_rows) {
		first_row -= (last_row - inp_rows);	/* bottom */
		last_row = inp_rows;
	    }
	}
	else {
	    first_row = 1;	/* top */
	    last_row = first_row + inp_seg_r + overlap1 * 2 - 1;
	}
	if ((last_row > inp_rows) || (first_row < 1)) {
	    fprintf(stderr, "Row overlap too large!\n");
	    return -1;
	}
	input_data(params, first_row, last_row, in_points, fdsmooth, fdinp,
		   inp_rows, inp_cols, zmin, inp_ns_res, inp_ew_res);

	for (j = 1; j <= div; j++) {	/* input and output cols */
	    if (j <= div - rem_out_col)
		n_cols = out_seg_c;
	    else
		n_cols = out_seg_c + 1;
	    inp_c = inp_seg_c;

	    ngstc = out_check_cols + 1;	/* first output col of the segment */
	    nszc = ngstc + n_cols - 1;	/* last output col of the segment */
	    x_or = (ngstc - 1) * ew_res;	/* x origin of the segment */

	    first_col = (int)(x_or / inp_ew_res) + 1;
	    if (first_col > overlap1) {
		first_col -= overlap1;	/* middle */
		last_col = first_col + inp_seg_c + overlap1 * 2 - 1;
		if (last_col > inp_cols) {
		    first_col -= (last_col - inp_cols);	/* right */
		    last_col = inp_cols;
		}
	    }
	    else {
		first_col = 1;	/* left */
		last_col = first_col + inp_seg_c + overlap1 * 2 - 1;
	    }
	    if ((last_col > inp_cols) || (first_col < 1)) {
		fprintf(stderr, "Column overlap too large!\n");
		return -1;
	    }
	    m = 0;
	    /* Getting points for interpolation (translated) */

	    xm = nszc * ew_res;
	    ym = nszr * ns_res;
	    data = (struct quaddata *)quad_data_new(x_or, y_or, xm, ym,
						    nszr - ngstr + 1,
						    nszc - ngstc + 1, 0,
						    params->KMAX2);
	    new_comp = 0;

	    for (k = 0; k <= last_row - first_row; k++) {
		for (l = first_col - 1; l < last_col; l++) {
		    index = k * inp_cols + l;
		    if (!Rast_is_f_null_value(&(in_points[index].z))) {
			/* if the point is inside the segment (not overlapping) */
			if ((in_points[index].x - x_or >= 0) &&
			    (in_points[index].y - y_or >= 0) &&
			    ((nszc - 1) * ew_res - in_points[index].x >= 0) &&
			    ((nszr - 1) * ns_res - in_points[index].y >= 0))
			    total_points += 1;
			data->points[m].x =
			    (in_points[index].x - x_or) / (*dnorm);
			data->points[m].y =
			    (in_points[index].y - y_or) / (*dnorm);
			/*            data->points[m].z = (double) (in_points[index].z) / (*dnorm); */
			data->points[m].z = (double)(in_points[index].z);
			data->points[m].sm = in_points[index].smooth;
			m++;
		    }
		    else
			new_comp = 1;

		    /*          fprintf(stderr,"%f,%f,%f
		       zmin=%f\n",in_points[index].x,in_points[index].y,in_points[index].z,zmin);
		     */
		}
	    }
	    /*      fprintf (stdout,"m,index:%di,%d\n",m,index); */
	    if (m <= params->KMAX2)
		data->n_points = m;
	    else
		data->n_points = params->KMAX2;
	    out_check_cols += n_cols;
	    inp_check_cols += inp_c;
	    cursegm = (i - 1) * div + j - 1;

	    /* show before to catch 0% */
	    if (totsegm != 0) {
		G_percent(cursegm, totsegm, 1);
	    }
	    if (m == 0) {
		/*
		 * fprintf(stderr,"Warning: segment with zero points encountered,
		 * insrease overlap\n");
		 */
		write_zeros(params, data, offset1);
	    }
	    else {
		if (new_comp) {
		    if (new_first) {
			new_first = 0;
			if (!b) {
			    if (!(b = G_alloc_vector(params->KMAX2 + 2))) {
				fprintf(stderr,
					"Cannot allocate memory for b\n");
				return -1;
			    }
			}
			if (!(new_indx = G_alloc_ivector(params->KMAX2 + 1))) {
			    fprintf(stderr,
				    "Cannot allocate memory for new_indx\n");
			    return -1;
			}
			if (!
			    (new_matrix =
			     G_alloc_matrix(params->KMAX2 + 1,
					    params->KMAX2 + 1))) {
			    fprintf(stderr,
				    "Cannot allocate memory for new_matrix\n");
			    return -1;
			}
		    }		/*new_first */
		    if (params->
			matrix_create(params, data->points, data->n_points,
				      new_matrix, new_indx) < 0)
			return -1;

		    for (i1 = 0; i1 < m; i1++) {
			b[i1 + 1] = data->points[i1].z;
		    }
		    b[0] = 0.;
		    G_lubksb(new_matrix, data->n_points + 1, new_indx, b);

		    params->check_points(params, data, b, ertot, zmin,
					 *dnorm, triple);

		    if (params->grid_calc(params, data, bitmask,
					  zmin, zmax, zminac, zmaxac, gmin,
					  gmax, c1min, c1max, c2min, c2max,
					  ertot, b, offset1, *dnorm) < 0) {

			fprintf(stderr, "interpolate() failed\n");
			return -1;
		    }
		}		/*new_comp */
		else {
		    if (first) {
			first = 0;
			if (!b) {
			    if (!(b = G_alloc_vector(params->KMAX2 + 2))) {
				fprintf(stderr,
					"Cannot allocate memory for b\n");
				return -1;
			    }
			}
			if (!(indx = G_alloc_ivector(params->KMAX2 + 1))) {
			    fprintf(stderr,
				    "Cannot allocate memory for indx\n");
			    return -1;
			}
			if (!
			    (matrix =
			     G_alloc_matrix(params->KMAX2 + 1,
					    params->KMAX2 + 1))) {
			    fprintf(stderr,
				    "Cannot allocate memory for matrix\n");
			    return -1;
			}
		    }		/* first */
		    if (params->
			matrix_create(params, data->points, data->n_points,
				      matrix, indx) < 0)
			return -1;
		    /*        } here it was bug */
		    for (i1 = 0; i1 < m; i1++)
			b[i1 + 1] = data->points[i1].z;
		    b[0] = 0.;
		    G_lubksb(matrix, data->n_points + 1, indx, b);

		    params->check_points(params, data, b, ertot, zmin,
					 *dnorm, triple);

		    if (params->grid_calc(params, data, bitmask,
					  zmin, zmax, zminac, zmaxac, gmin,
					  gmax, c1min, c1max, c2min, c2max,
					  ertot, b, offset1, *dnorm) < 0) {

			fprintf(stderr, "interpolate() failed\n");
			return -1;
		    }
		}
	    }
	    if (data) {
		G_free(data->points);
		G_free(data);
	    }
	    /*
	     * cursegm++;
	     */
	}

	inp_check_rows += inp_r;
	out_check_rows += n_rows;
    }

    /* run one last time after the loop is done to catch 100% */
    if (totsegm != 0)
	G_percent(1, 1, 1);	/* cursegm doesn't get to totsegm so we force 100% */

    /*
     * if (b) G_free_vector(b); if (indx) G_free_ivector(indx); if (matrix)
     * G_free_matrix(matrix);
     */
    fprintf(stderr, "dnorm in ressegm after grid before out2= %f \n", *dnorm);
    return total_points;
}

/* input of data for interpolation and smoothing parameters */

static int input_data(struct interp_params *params,
		      int first_row, int last_row,
		      struct fcell_triple *points,
		      int fdsmooth, int fdinp,
		      int inp_rows, int inp_cols,
		      double zmin, double inp_ns_res, double inp_ew_res)
{
    double x, y, sm;		/* input data and smoothing */
    int m1, m2;			/* loop counters */
    static FCELL *cellinp = NULL;	/* cell buffer for input data */
    static FCELL *cellsmooth = NULL;	/* cell buffer for smoothing */


    if (!cellinp)
	cellinp = Rast_allocate_f_buf();
    if (!cellsmooth)
	cellsmooth = Rast_allocate_f_buf();

    for (m1 = 0; m1 <= last_row - first_row; m1++) {
	Rast_get_f_row(fdinp, cellinp, inp_rows - m1 - first_row);
	if (fdsmooth >= 0)
	    Rast_get_f_row(fdsmooth, cellsmooth, inp_rows - m1 - first_row);

	y = params->y_orig + (m1 + first_row - 1 + 0.5) * inp_ns_res;
	for (m2 = 0; m2 < inp_cols; m2++) {
	    x = params->x_orig + (m2 + 0.5) * inp_ew_res;
	    /*
	     * z = cellinp[m2]*params->zmult;
	     */
	    if (fdsmooth >= 0)
		sm = (double)cellsmooth[m2];
	    else
		sm = 0.01;

	    points[m1 * inp_cols + m2].x = x - params->x_orig;
	    points[m1 * inp_cols + m2].y = y - params->y_orig;
	    if (!Rast_is_f_null_value(cellinp + m2)) {
		points[m1 * inp_cols + m2].z =
		    cellinp[m2] * params->zmult - zmin;
	    }
	    else {
		Rast_set_f_null_value(&(points[m1 * inp_cols + m2].z), 1);
	    }

	    /*              fprintf (stdout,"sm: %f\n",sm); */

	    points[m1 * inp_cols + m2].smooth = sm;
	}
    }
    return 1;
}

static int write_zeros(struct interp_params *params,
		       struct quaddata *data,	/* given segment */
		       off_t offset1		/* offset for temp file writing */
    )
{

    /*
     * C C       INTERPOLATION BY FUNCTIONAL METHOD : TPS + complete regul.
     * c
     */
    double x_or = data->x_orig;
    double y_or = data->y_orig;
    int n_rows = data->n_rows;
    int n_cols = data->n_cols;
    int cond1, cond2;
    int k, l;
    int ngstc, nszc, ngstr, nszr;
    off_t offset, offset2;
    double ns_res, ew_res;

    ns_res = (((struct quaddata *)(data))->ymax -
	      ((struct quaddata *)(data))->y_orig) / data->n_rows;
    ew_res = (((struct quaddata *)(data))->xmax -
	      ((struct quaddata *)(data))->x_orig) / data->n_cols;

    cond2 = ((params->adxx != NULL) || (params->adyy != NULL) ||
	     (params->adxy != NULL));
    cond1 = ((params->adx != NULL) || (params->ady != NULL) || cond2);

    ngstc = (int)(x_or / ew_res + 0.5) + 1;
    nszc = ngstc + n_cols - 1;
    ngstr = (int)(y_or / ns_res + 0.5) + 1;
    nszr = ngstr + n_rows - 1;

    for (k = ngstr; k <= nszr; k++) {
	offset = offset1 * (k - 1);	/* rows offset */
	for (l = ngstc; l <= nszc; l++) {
	    /*
	     * params->az[l] = 0.;
	     */
	    Rast_set_d_null_value(params->az + l, 1);
	    if (cond1) {
		/*
		 * params->adx[l] = (FCELL)0.; params->ady[l] = (FCELL)0.;
		 */
		Rast_set_d_null_value(params->adx + l, 1);
		Rast_set_d_null_value(params->ady + l, 1);
		if (cond2) {
		    Rast_set_d_null_value(params->adxx + l, 1);
		    Rast_set_d_null_value(params->adyy + l, 1);
		    Rast_set_d_null_value(params->adxy + l, 1);
		    /*
		     * params->adxx[l] = (FCELL)0.; params->adyy[l] = (FCELL)0.;
		     * params->adxy[l] = (FCELL)0.;
		     */
		}
	    }
	}
	offset2 = (offset + ngstc - 1) * sizeof(FCELL);
	if (params->wr_temp(params, ngstc, nszc, offset2) < 0)
	    return -1;
    }
    return 1;
}
