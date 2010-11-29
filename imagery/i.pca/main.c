
/****************************************************************************
 *
 * MODULE:       i.pca
 *
 * AUTHOR(S):    Original author Center for Space Research (Uni. of TX)
 *               Rewritten by Brad Douglas <rez touchofmadness com>
 *
 * PURPOSE:      Principal Component Analysis transform of satellite data.
 *
 * COPYRIGHT:    (C) 2004-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gmath.h>
#include <grass/glocale.h>
#include "local_proto.h"


#undef PCA_DEBUG


/* function prototypes */
static CELL round_c(double);
static int set_output_scale(struct Option *, int *, int *, int *);
static int calc_mu(int *, double *, int);
static int calc_covariance(int *, double **, double *, int);
static int write_pca(double **, int *, char *, int, int, int, int);

#ifdef PCA_DEBUG
static int dump_eigen(int, double **, double *);
#endif


int main(int argc, char *argv[])
{
    int i, j;			/* Loop control variables */
    int bands;			/* Number of image bands */
    double *mu;			/* Mean vector for image bands */
    double **covar;		/* Covariance Matrix */
    double *eigval;
    double **eigmat;
    int *inp_fd;
    int scale, scale_max, scale_min;

    struct GModule *module;
    struct Option *opt_in, *opt_out, *opt_scale;

    /* initialize GIS engine */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("image transformation"));
    G_add_keyword(_("PCA"));
    module->description = _("Principal components analysis (PCA) "
			    "for image processing.");

    /* Define options */
    opt_in = G_define_standard_option(G_OPT_R_INPUTS);
    opt_in->description = _("Name of two or more input raster maps");

    opt_out = G_define_option();
    opt_out->label = _("Base name for output raster maps");
    opt_out->description =
	_("A numerical suffix will be added for each component map");
    opt_out->key = "output_prefix";
    opt_out->type = TYPE_STRING;
    opt_out->key_desc = "string";
    opt_out->required = YES;

    opt_scale = G_define_option();
    opt_scale->key = "rescale";
    opt_scale->type = TYPE_INTEGER;
    opt_scale->key_desc = "min,max";
    opt_scale->required = NO;
    opt_scale->answer = "0,255";
    opt_scale->label =
	_("Rescaling range for output maps");
    opt_scale->description =
	_("For no rescaling use 0,0");
    opt_scale->guisection = _("Rescale");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* determine number of bands passed in */
    for (bands = 0; opt_in->answers[bands] != NULL; bands++) ;

    if (bands < 2)
	G_fatal_error(_("Sorry, at least 2 input bands must be provided"));

    /* default values */
    scale = 1;
    scale_min = 0;
    scale_max = 255;

    /* get scale parameters */
    set_output_scale(opt_scale, &scale, &scale_min, &scale_max);

    /* allocate memory */
    covar = G_alloc_matrix(bands, bands);
    mu = G_alloc_vector(bands);
    inp_fd = G_alloc_ivector(bands);
    eigmat = G_alloc_matrix(bands, bands);
    eigval = G_alloc_vector(bands);

    /* open and check input/output files */
    for (i = 0; i < bands; i++) {
	char tmpbuf[128];

	sprintf(tmpbuf, "%s.%d", opt_out->answer, i + 1);
	G_check_input_output_name(opt_in->answers[i], tmpbuf, GR_FATAL_EXIT);

	inp_fd[i] = Rast_open_old(opt_in->answers[i], "");
    }

    G_verbose_message(_("Calculating covariance matrix..."));
    calc_mu(inp_fd, mu, bands);

    calc_covariance(inp_fd, covar, mu, bands);

    for (i = 0; i < bands; i++) {
	for (j = 0; j < bands; j++) {
	    covar[i][j] =
		covar[i][j] /
		((double)((Rast_window_rows() * Rast_window_cols()) - 1));
	    G_debug(3, "covar[%d][%d] = %f", i, j, covar[i][j]);
	}
    }

    G_math_d_copy(covar[0], eigmat[0], bands*bands);
    G_debug(1, "Calculating eigenvalues and eigenvectors...");
    G_math_eigen(eigmat, eigval, bands);

#ifdef PCA_DEBUG
    /* dump eigen matrix and eigen values */
    dump_eigen(bands, eigmat, eigval);
#endif

    G_debug(1, "Ordering eigenvalues in descending order...");
    G_math_egvorder(eigval, eigmat, bands);

    G_debug(1, "Transposing eigen matrix...");
    G_math_d_A_T(eigmat, bands);

    /* write output images */
    write_pca(eigmat, inp_fd, opt_out->answer, bands, scale, scale_min,
	      scale_max);

    /* write colors and history to output */
    for (i = 0; i < bands; i++) {
	char outname[80];

	sprintf(outname, "%s.%d", opt_out->answer, i + 1);

	/* write colors and history to file */
	write_support(bands, outname, eigmat, eigval);

	/* close output file */
	Rast_unopen(inp_fd[i]);
    }
    
    /* free memory */
    G_free_matrix(covar);
    G_free_vector(mu);
    G_free_ivector(inp_fd);
    G_free_matrix(eigmat);
    G_free_vector(eigval);

    exit(EXIT_SUCCESS);
}


static CELL round_c(double x)
{
    CELL n;

    if (x >= 0.0)
	n = x + .5;
    else {
	n = -x + .5;
	n = -n;
    }

    return n;
}


static int
set_output_scale(struct Option *scale_opt, int *scale, int *scale_min,
		 int *scale_max)
{
    if (scale_opt->answer) {
	sscanf(scale_opt->answers[0], "%d", (int *)scale_min);
	sscanf(scale_opt->answers[1], "%d", (int *)scale_max);

	if (*scale_min == *scale_max) {
	    if (*scale_min == 0)
		*scale = 0;
	    else {
		G_warning(_("Scale range length should be > 0. "
			    "Using default values: 0,255."));
		*scale_min = 0;
		*scale_max = 255;
	    }
	}

	if (*scale_max < *scale_min) {
	    int tmp_scale = *scale_max;

	    *scale_max = *scale_min;
	    *scale_min = tmp_scale;
	}
    }

    return 0;
}


static int calc_mu(int *fds, double *mu, int bands)
{
    int i;
    int rows = Rast_window_rows();
    int cols = Rast_window_cols();
    void *rowbuf = NULL;

    for (i = 0; i < bands; i++) {
	RASTER_MAP_TYPE maptype;
	int row, col;
	double sum = 0.;

	maptype = Rast_get_map_type(fds[i]);

	/* don't assume each image is of the same type */
	if (rowbuf)
	    G_free(rowbuf);
	if ((rowbuf = Rast_allocate_buf(maptype)) == NULL)
	    G_fatal_error(_("Unable allocate memory for row buffer"));

	G_message(_("Computing means for band %d..."), i + 1);
	for (row = 0; row < rows; row++) {
	    void *ptr = rowbuf;

	    G_percent(row, rows - 1, 2);

	    Rast_get_row(fds[i], rowbuf, row, maptype);

	    for (col = 0; col < cols; col++) {
		/* skip null cells */
		if (Rast_is_null_value(ptr, maptype)) {
		    ptr = G_incr_void_ptr(ptr, Rast_cell_size(maptype));
		    continue;
		}

		sum += Rast_get_d_value(ptr, maptype);
		ptr = G_incr_void_ptr(ptr, Rast_cell_size(maptype));
	    }
	}

	mu[i] = sum / (double)(rows * cols);
    }

    if (rowbuf)
	G_free(rowbuf);

    return 0;
}


static int calc_covariance(int *fds, double **covar, double *mu, int bands)
{
    int j, k;
    int rows = Rast_window_rows();
    int cols = Rast_window_cols();
    int row, col;

    for (j = 0; j < bands; j++) {
	RASTER_MAP_TYPE maptype = Rast_get_map_type(fds[j]);
	void *rowbuf1 = NULL;
	void *rowbuf2 = NULL;

	/* don't assume each image is of the same type */
	if (rowbuf1)
	    G_free(rowbuf1);
	if ((rowbuf1 = Rast_allocate_buf(maptype)) == NULL)
	    G_fatal_error(_("Unable allocate memory for row buffer"));

	G_message(_("Computing row %d (of %d) of covariance matrix..."),
		  j + 1, bands);
	for (row = 0; row < rows; row++) {
	    void *ptr1, *ptr2;

	    G_percent(row, rows - 1, 2);

	    Rast_get_row(fds[j], rowbuf1, row, maptype);

	    for (k = j; k < bands; k++) {
		RASTER_MAP_TYPE maptype2 = Rast_get_map_type(fds[k]);

		/* don't assume each image is of the same type */
		if (rowbuf2)
		    G_free(rowbuf2);
		if ((rowbuf2 = Rast_allocate_buf(maptype2)) == NULL)
		    G_fatal_error(_("Unable to allocate memory for row buffer"));

		Rast_get_row(fds[k], rowbuf2, row, maptype2);

		ptr1 = rowbuf1;
		ptr2 = rowbuf2;

		for (col = 0; col < cols; col++) {
		    /* skip null cells */
		    if (Rast_is_null_value(ptr1, maptype) ||
			Rast_is_null_value(ptr2, maptype2)) {
			ptr1 = G_incr_void_ptr(ptr1, Rast_cell_size(maptype));
			ptr2 = G_incr_void_ptr(ptr2, Rast_cell_size(maptype2));
			continue;
		    }

		    covar[j][k] +=
			((double)Rast_get_d_value(ptr1, maptype) -
			 mu[j]) * ((double)Rast_get_d_value(ptr2,
						   maptype2) - mu[k]);

		    ptr1 = G_incr_void_ptr(ptr1, Rast_cell_size(maptype));
		    ptr2 = G_incr_void_ptr(ptr2, Rast_cell_size(maptype2));
		}

		covar[k][j] = covar[j][k];
	    }
	}
    }

    return 0;
}


static int
write_pca(double **eigmat, int *inp_fd, char *out_basename,
	  int bands, int scale, int scale_min, int scale_max)
{
    int i, j;
    void *outbuf, *outptr;
    double min = 0.;
    double max = 0.;
    double old_range = 0.;
    double new_range = 0.;
    int rows = Rast_window_rows();
    int cols = Rast_window_cols();
    int cell_mapsiz = Rast_cell_size(CELL_TYPE);
    int dcell_mapsiz = Rast_cell_size(DCELL_TYPE);
    DCELL *d_buf;

    /* 2 passes for rescale.  1 pass for no rescale */
    int PASSES = (scale) ? 2 : 1;

    /* temporary row storage */
    d_buf = (DCELL *) G_malloc(cols * sizeof(double));

    /* allocate memory for output row buffer */
    outbuf = (scale) ? Rast_allocate_buf(CELL_TYPE) :
	Rast_allocate_buf(DCELL_TYPE);

    if (!outbuf)
	G_fatal_error(_("Unable to allocate memory for raster row"));

    for (i = 0; i < bands; i++) {
	char name[100];
	int out_fd;
	int pass;

	sprintf(name, "%s.%d", out_basename, i + 1);

	G_message(_("Transforming <%s>..."), name);

	/* open a new file for output */
	if (scale)
	    out_fd = Rast_open_c_new(name);
	else {
	    out_fd = Rast_open_fp_new(name);
	    Rast_set_fp_type(DCELL_TYPE);
	}

	for (pass = 1; pass <= PASSES; pass++) {
	    void *rowbuf = NULL;
	    int row, col;

	    if (scale && (pass == PASSES)) {
		G_message(_("Rescaling <%s> to range %d,%d..."),
			  name, scale_min, scale_max);

		old_range = max - min;
		new_range = (double)(scale_max - scale_min);
	    }

	    for (row = 0; row < rows; row++) {
		void *rowptr;

		G_percent(row, rows, 2);

		/* reset d_buf */
		for (col = 0; col < cols; col++)
		    d_buf[col] = 0.;

		for (j = 0; j < bands; j++) {
		    RASTER_MAP_TYPE maptype =
			Rast_get_map_type(inp_fd[j]);

		    /* don't assume each image is of the same type */
		    if (rowbuf)
			G_free(rowbuf);
		    if (!(rowbuf = Rast_allocate_buf(maptype)))
			G_fatal_error(_("Unable allocate memory for row buffer"));

		    Rast_get_row(inp_fd[j], rowbuf, row, maptype);

		    rowptr = rowbuf;
		    outptr = outbuf;

		    /* add into the output cell eigmat[i][j] * corresp cell 
		     * of j-th band for current j */
		    for (col = 0; col < cols; col++) {
			/* handle null cells */
			if (Rast_is_null_value(rowptr, maptype)) {
			    if (scale) {
				Rast_set_null_value(outptr, 1, CELL_TYPE);
				outptr = G_incr_void_ptr(outptr, cell_mapsiz);
			    }
			    else {
				Rast_set_null_value(outptr, 1, DCELL_TYPE);
				outptr =
				    G_incr_void_ptr(outptr, dcell_mapsiz);
			    }

			    rowptr =
				G_incr_void_ptr(rowptr,
						Rast_cell_size(maptype));
			    continue;
			}

			/* corresp. cell of j-th band */
			d_buf[col] +=
			    eigmat[i][j] * Rast_get_d_value(rowptr,
								maptype);

			/* the cell entry is complete */
			if (j == (bands - 1)) {
			    if (scale && (pass == 1)) {
				if ((row == 0) && (col == 0))
				    min = max = d_buf[0];

				if (d_buf[col] < min)
				    min = d_buf[col];

				if (d_buf[col] > max)
				    max = d_buf[col];
			    }
			    else if (scale) {

				if (min == max) {
				    Rast_set_c_value(outptr, 1,
							 CELL_TYPE);
				}
				else {
				    /* map data to 0, (new_range-1) and then adding new_min */
				    CELL tmpcell =
					round_c((new_range *
						 (d_buf[col] -
						  min) / old_range) +
						scale_min);

				    Rast_set_c_value(outptr, tmpcell,
							 CELL_TYPE);
				}
			    }
			    else {	/* (!scale) */

				Rast_set_d_value(outptr, d_buf[col],
						     DCELL_TYPE);
			    }
			}

			outptr = (scale) ?
			    G_incr_void_ptr(outptr, cell_mapsiz) :
			    G_incr_void_ptr(outptr, dcell_mapsiz);

			rowptr =
			    G_incr_void_ptr(rowptr, Rast_cell_size(maptype));
		    }
		}		/* for j = 0 to bands */

		if (pass == PASSES) {
		    if (scale)
			Rast_put_row(out_fd, outbuf, CELL_TYPE);
		    else
			Rast_put_row(out_fd, outbuf, DCELL_TYPE);
		}
	    }

	    G_percent(row, rows, 2);

	    /* close output file */
	    if (pass == PASSES)
		Rast_close(out_fd);
	}
    }

    if (d_buf)
	G_free(d_buf);
    if (outbuf)
	G_free(outbuf);

    return 0;
}


#ifdef PCA_DEBUG
static int dump_eigen(int bands, double **eigmat, double *eigval)
{
    int i, j;

    for (i = 0; i < bands; i++) {
	for (j = 0; j < bands; j++)
	    fprintf(stderr, "%f  ", eigmat[i][j]);

	fprintf(stderr, "\n");
    }

    for (i = 0; i < bands; i++)
	fprintf(stderr, "%f  ", eigval[i]);

    fprintf(stderr, "\n");

    return 0;
}
#endif
