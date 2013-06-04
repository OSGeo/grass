
/****************************************************************************
 *
 * MODULE:       i.pca
 *
 * AUTHOR(S):    Original author Center for Space Research (Uni. of TX)
 *               Rewritten by Brad Douglas <rez touchofmadness com>
 *               NULL value/MASK handling and speed up by Markus Metz
 *
 * PURPOSE:      Principal Component Analysis transform of raster data.
 *
 * COPYRIGHT:    (C) 2004-2011 by the GRASS Development Team
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
#include <grass/imagery.h>
#include <grass/gmath.h>
#include <grass/glocale.h>
#include "local_proto.h"


#undef PCA_DEBUG


/* function prototypes */
static CELL round_c(double);
static int set_output_scale(struct Option *, int *, int *, int *);
static int calc_mu_cov(int *, double **, double *, double *, int);
static int write_pca(double **, double *, double *, int *, char *, int,
                     int, int, int, int);

#ifdef PCA_DEBUG
static int dump_eigen(int, double **, double *);
#endif


int main(int argc, char *argv[])
{
    int i;			/* Loop control variables */
    int bands;			/* Number of image bands */
    int pcbands;		/* Number of pc scores to use for filtering */
    int pcperc;			/* cumulative percent to use for filtering */
    double *mu;			/* Mean vector for image bands */
    double *stddev;		/* Stddev vector for image bands */
    double **covar;		/* Covariance Matrix */
    double *eigval;
    double **eigmat;
    int *inp_fd;
    int scale, scale_max, scale_min;
    struct Ref ref;
    const char *mapset;

    struct GModule *module;
    struct Option *opt_in, *opt_out, *opt_scale, *opt_filt;
    struct Flag *flag_norm, *flag_filt;

    /* initialize GIS engine */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("transformation"));
    G_add_keyword(_("PCA"));
    module->description = _("Principal components analysis (PCA) "
			    "for image processing.");

    /* Define options */
    opt_in = G_define_standard_option(G_OPT_R_INPUTS);
    opt_in->description = _("Name of two or more input raster maps or imagery group");

    opt_out = G_define_option();
    opt_out->label = _("Prefix for output raster maps");
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
    
    opt_filt = G_define_option();
    opt_filt->key = "percent";
    opt_filt->type = TYPE_INTEGER;
    opt_filt->required = NO;
    opt_filt->options = "50-99";
    opt_filt->answer = "99";
    opt_filt->label =
	_("Cumulative percent importance for filtering");
    opt_filt->guisection = _("Filter");

    flag_norm = G_define_flag();
    flag_norm->key = 'n';
    flag_norm->label = (_("Normalize (center and scale) input maps"));
    flag_norm->description = (_("Default: center only"));
    
    flag_filt = G_define_flag();
    flag_filt->key = 'f';
    flag_filt->label = (_("Output will be filtered input bands"));
    flag_filt->description = (_("Applies inverse PCA after PCA"));
    flag_filt->guisection = _("Filter");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* determine number of bands passed in */
    for (bands = 0; opt_in->answers[bands] != NULL; bands++) ;

    /* input can be either several raster maps or a group */
    if (bands > 1) {
	char name[GNAME_MAX];
	
	I_init_group_ref(&ref);
	for (i = 0; opt_in->answers[i] != NULL; i++) {
	    /* strip @mapset, do not modify opt_in->answers */
	    strcpy(name, opt_in->answers[i]);
	    mapset = G_find_raster(name, "");
	    if (!mapset)
		G_fatal_error(_("Raster map <%s> not found"),
			      opt_in->answers[i]);
	    /* Add input to group. */
	    I_add_file_to_group_ref(name, mapset, &ref);
	}
    }
    else {
	/* Maybe input is group. Try to read group file */
	if (I_get_group_ref(opt_in->answer, &ref) != 1)
	    G_fatal_error(_("Group <%s> not found"),
			  opt_in->answer);
    }
    bands = ref.nfiles;

    if (bands < 2)
	G_fatal_error(_("Sorry, at least 2 input bands must be provided"));

    /* default values */
    scale = 1;
    scale_min = 0;
    scale_max = 255;

    /* get scale parameters */
    set_output_scale(opt_scale, &scale, &scale_min, &scale_max);

    /* filter threshold */
    pcperc = -1;
    if (flag_filt->answer) {
	pcperc = atoi(opt_filt->answer);
	if (pcperc < 0)
	    G_fatal_error(_("'%s' must be positive"), opt_filt->key);
	if (pcperc > 99)
	    G_fatal_error(_("'%s' must be < 100"), opt_filt->key);
    }

    /* allocate memory */
    covar = G_alloc_matrix(bands, bands);
    mu = G_alloc_vector(bands);
    inp_fd = G_alloc_ivector(bands);
    eigmat = G_alloc_matrix(bands, bands);
    eigval = G_alloc_vector(bands);
    
    if (flag_norm->answer)
	stddev = G_alloc_vector(bands);
    else
	stddev = NULL;

    /* open and check input/output files */
    for (i = 0; i < bands; i++) {
	char tmpbuf[GNAME_MAX];

	sprintf(tmpbuf, "%s.%d", opt_out->answer, i + 1);
	G_check_input_output_name(ref.file[i].name, tmpbuf, G_FATAL_EXIT);

	inp_fd[i] = Rast_open_old(ref.file[i].name, ref.file[i].mapset);
    }

    if (!calc_mu_cov(inp_fd, covar, mu, stddev, bands))
	G_fatal_error(_("No non-null values"));

    G_math_d_copy(covar[0], eigmat[0], bands * bands);
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

    pcbands = 0;
    if (flag_filt->answer) {
	double eigval_total = 0.0;
	double eigval_perc = 0.0;

	for (i = 0; i < bands; i++) {
	    eigval_total += eigval[i];
	}
	for (i = 0; i < bands; i++) {
	    eigval_perc += eigval[i] * 100. / eigval_total;
	    pcbands++;
	    if (eigval_perc > pcperc)
		break;
	}

	/* filtering has an effect only if at least one PC is removed  */
	if (pcbands == bands)
	    pcbands--;
	if (pcbands < 2)
	    G_fatal_error(_("Not enough principal components left for filtering"));

	G_message(_("Using %d of %d principal components for filtering"), pcbands, bands);
	scale = 0;
    }

    /* write output images */
    write_pca(eigmat, mu, stddev, inp_fd, opt_out->answer, bands,
              scale, scale_min, scale_max, pcbands);

    /* write colors and history to output */
    for (i = 0; i < bands; i++) {
	char outname[GNAME_MAX];

	/* close input files */
	Rast_unopen(inp_fd[i]);

	sprintf(outname, "%s.%d", opt_out->answer, i + 1);

	/* write colors and history to file */
	if (flag_filt->answer)
	    write_support(bands, ref.file[i].name, outname, eigmat, eigval);
	else
	    write_support(bands, NULL, outname, eigmat, eigval);
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


static int calc_mu_cov(int *fds, double **covar, double *mu, 
                           double *stddev, int bands)
{
    int i, j;
    int row, col;
    int rows = Rast_window_rows();
    int cols = Rast_window_cols();
    off_t count = 0;
    DCELL **rowbuf = (DCELL **) G_malloc(bands * sizeof(DCELL *));
    double **sum2 = (double **)G_calloc(bands, sizeof(double *));
    double *sumsq, *sd, *sum;

    if (stddev) {
	sumsq = (double *)G_calloc(bands, sizeof(double));
	sd = (double *)G_calloc(bands, sizeof(double));
    }
    else {
	sumsq = NULL;
	sd = NULL;
    }

    for (i = 0; i < bands; i++) {
	rowbuf[i] = Rast_allocate_d_buf();
	sum2[i] = (double *)G_calloc(bands, sizeof(double));
    }
    sum = mu;

    G_message(_("Computing covariance matrix..."));

    for (row = 0; row < rows; row++) {
	G_percent(row, rows, 2);
	for (i = 0; i < bands; i++)
	    Rast_get_d_row(fds[i], rowbuf[i], row);

	for (col = 0; col < cols; col++) {
	    /* ignore cells where any of the maps has null value */
	    for (i = 0; i < bands; i++)
		if (Rast_is_d_null_value(&rowbuf[i][col]))
		    break;
	    if (i != bands)
		continue;
	    count++;
	    for (i = 0; i < bands; i++) {

		sum[i] += rowbuf[i][col];
		if (stddev)
		    sumsq[i] += rowbuf[i][col] * rowbuf[i][col];

		for (j = 0; j <= i; j++)
		    sum2[i][j] += rowbuf[i][col] * rowbuf[j][col];
	    }
	}
    }
    G_percent(1, 1, 1);
    
    if (count < 2)
	return 0;

    for (i = 0; i < bands; i++) {
	if (stddev) {
	    sd[i] = sqrt(count * sumsq[i] - sum[i] * sum[i]);
	    stddev[i] = sqrt((sumsq[i] - sum[i] * sum[i] / count) /
	                     (count - 1));
	}
	for (j = 0; j <= i; j++) {
	    if (stddev)
		covar[i][j] = (count * sum2[i][j] - sum[i] * sum[j]) /
		              (sd[i] * sd[j]);
	    else
		covar[i][j] = (sum2[i][j] - sum[i] * sum[j] / count) /
		              (count - 1);
	    G_debug(3, "covar[%d][%d] = %f", i, j, covar[i][j]);
	    if (j != i)
		covar[j][i] = covar[i][j];
	}

	G_free(sum2[i]);
	G_free(rowbuf[i]);
    }
    for (i = 0; i < bands; i++)
	mu[i] = sum[i] / count;

    G_free(rowbuf);
    
    G_free(sum2);
    if (sd)
	G_free(sd);
    if (sumsq)
	G_free(sumsq);

    return 1;
}


static int
write_pca(double **eigmat, double *mu, double *stddev,
          int *inp_fd, char *out_basename, int bands, 
	  int scale, int scale_min, int scale_max, int fbands)
{
    int i, j;
    void **outbuf = (void **) G_malloc(bands * sizeof(void *));
    void **outptr = (void **) G_malloc(bands * sizeof(void *));
    double *min = (double *) G_malloc(bands * sizeof(double));
    double *max = (double *) G_malloc(bands * sizeof(double));
    double *old_range = (double *) G_calloc(bands, sizeof(double));
    double new_range = 0.;
    int pass;
    int rows = Rast_window_rows();
    int cols = Rast_window_cols();
    /* why CELL_TYPE when scaling output ? */
    int outmap_type = (scale) ? CELL_TYPE : DCELL_TYPE;
    int outcell_mapsiz = Rast_cell_size(outmap_type);
    int *out_fd = (int *) G_malloc(bands * sizeof(int));
    DCELL **inbuf = (DCELL **) G_malloc(bands * sizeof(DCELL *));
    DCELL *pcs = NULL;

    /* 2 passes for rescale.  1 pass for no rescale */
    int PASSES = (scale) ? 2 : 1;

    if (fbands)
	pcs = (DCELL *) G_malloc(fbands * sizeof(DCELL));

    /* allocate memory for row buffers */
    for (i = 0; i < bands; i++) {
	char name[GNAME_MAX];

	/* open output raster maps */
	sprintf(name, "%s.%d", out_basename, i + 1);
	out_fd[i] = Rast_open_new(name, outmap_type);

	inbuf[i] = Rast_allocate_d_buf();
	outbuf[i] = Rast_allocate_buf(outmap_type);
	min[i] = max[i] = old_range[i] = 0;
    }

    for (pass = 1; pass <= PASSES; pass++) {
	int row, col;
	int first = 1;

	if (scale && (pass == PASSES)) {
	    G_message(_("Rescaling to range %d,%d..."),
		      scale_min, scale_max);

	    for (i = 0; i < bands; i++)
		old_range[i] = max[i] - min[i];
	    new_range = (double)(scale_max - scale_min);
	}
	else {
	    G_message(_("Calculating principal components..."));
	}

	for (row = 0; row < rows; row++) {

	    G_percent(row, rows, 2);

	    for (i = 0; i < bands; i++) {
		Rast_get_d_row(inp_fd[i], inbuf[i], row);
		outptr[i] = outbuf[i];
	    }
	    for (col = 0; col < cols; col++) {
		/* ignore cells where any of the maps has null value */
		for (i = 0; i < bands; i++)
		    if (Rast_is_d_null_value(&inbuf[i][col]))
			break;
			
		if (i != bands) {
		    for (i = 0; i < bands; i++) {
			Rast_set_null_value(outptr[i], 1, outmap_type);
			outptr[i] =
			    G_incr_void_ptr(outptr[i], outcell_mapsiz);
		    }
		    continue;
		}
		
		if (fbands) {
		    /* calculate all PC scores */
		    for (i = 0; i < fbands; i++) {
			DCELL dval = 0.;

			for (j = 0; j < bands; j++) {
			    /* corresp. cell of j-th band */
			    if (stddev)
				dval += eigmat[i][j] * 
					((inbuf[j][col] - mu[j]) / stddev[j]);
			    else
				dval += eigmat[i][j] * (inbuf[j][col] - mu[j]);
			}
			pcs[i] = dval;
		    }
		}

		for (i = 0; i < bands; i++) {
		    DCELL dval = 0.;

		    if (fbands) {
			for (j = 0; j < fbands; j++) {
			    /* corresp. PC score of j-th band */
			    dval += eigmat[j][i] * pcs[j];
			}
			if (stddev)
			    dval = dval * stddev[i] + mu[i];
			else
			    dval += mu[i];
		    }
		    else {
			for (j = 0; j < bands; j++) {
			    /* corresp. cell of j-th band */
			    if (stddev)
				dval += eigmat[i][j] * 
					((inbuf[j][col] - mu[j]) / stddev[j]);
			    else
				dval += eigmat[i][j] * (inbuf[j][col] - mu[j]);
			}
		    }

		    /* the cell entry is complete */
		    if (scale && (pass == 1)) {
			if (first)
			    min[i] = max[i] = dval;
			if (dval < min[i])
			    min[i] = dval;

			if (dval > max[i])
			    max[i] = dval;
		    }
		    else if (scale) {

			if (min[i] == max[i]) {
			    Rast_set_c_value(outptr[i], 1,
						 CELL_TYPE);
			}
			else {
			    /* map data to 0, (new_range-1) and then adding new_min */
			    CELL tmpcell =
				round_c((new_range * (dval - min[i]) /
				         old_range[i]) + scale_min);

			    Rast_set_c_value(outptr[i], tmpcell,
						 outmap_type);
			}
		    }
		    else {	/* (!scale) */

			Rast_set_d_value(outptr[i], dval,
					     outmap_type);
		    }
		    outptr[i] = G_incr_void_ptr(outptr[i], outcell_mapsiz);
		}
		first = 0;
	    }
	    if (pass == PASSES) {
		for (i = 0; i < bands; i++)
		    Rast_put_row(out_fd[i], outbuf[i], outmap_type);
	    }
	}
	G_percent(1, 1, 1);

	/* close output file */
	if (pass == PASSES) {
	    for (i = 0; i < bands; i++) {
		Rast_close(out_fd[i]);
		G_free(inbuf[i]);
		G_free(outbuf[i]);
	    }
	}
    }

    G_free(inbuf);
    G_free(outbuf);
    G_free(outptr);
    G_free(min);
    G_free(max);
    G_free(old_range);

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
