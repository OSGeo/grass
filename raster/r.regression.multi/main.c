
/****************************************************************************
 *
 * MODULE:       r.regression.multi
 * 
 * AUTHOR(S):    Markus Metz
 * 
 * PURPOSE:      Calculates multiple linear regression from raster maps:
 *               y = b0 + b1*x1 + b2*x2 + ... +  bn*xn + e
 * 
 * COPYRIGHT:    (C) 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>

struct MATRIX
{
    int n;			/* SIZE OF THIS MATRIX (N x N) */
    double *v;
};

#define M(m,row,col) (m)->v[((row) * ((m)->n)) + (col)]

static int solvemat(struct MATRIX *m, double a[], double B[])
{
    int i, j, i2, j2, imark;
    double factor, temp;
    double pivot;		/* ACTUAL VALUE OF THE LARGEST PIVOT CANDIDATE */

    for (i = 0; i < m->n; i++) {
	j = i;

	/* find row with largest magnitude value for pivot value */

	pivot = M(m, i, j);
	imark = i;
	for (i2 = i + 1; i2 < m->n; i2++) {
	    temp = fabs(M(m, i2, j));
	    if (temp > fabs(pivot)) {
		pivot = M(m, i2, j);
		imark = i2;
	    }
	}

	/* if the pivot is very small then the points are nearly co-linear */
	/* co-linear points result in an undefined matrix, and nearly */
	/* co-linear points results in a solution with rounding error */

	if (pivot == 0.0) {
	    G_warning(_("Matrix is unsolvable"));
	    return 0;
	}

	/* if row with highest pivot is not the current row, switch them */

	if (imark != i) {
	    for (j2 = 0; j2 < m->n; j2++) {
		temp = M(m, imark, j2);
		M(m, imark, j2) = M(m, i, j2);
		M(m, i, j2) = temp;
	    }

	    temp = a[imark];
	    a[imark] = a[i];
	    a[i] = temp;
	}

	/* compute zeros above and below the pivot, and compute
	   values for the rest of the row as well */

	for (i2 = 0; i2 < m->n; i2++) {
	    if (i2 != i) {
		factor = M(m, i2, j) / pivot;
		for (j2 = j; j2 < m->n; j2++)
		    M(m, i2, j2) -= factor * M(m, i, j2);
		a[i2] -= factor * a[i];
	    }
	}
    }

    /* SINCE ALL OTHER VALUES IN THE MATRIX ARE ZERO NOW, CALCULATE THE
       COEFFICIENTS BY DIVIDING THE COLUMN VECTORS BY THE DIAGONAL VALUES. */

    for (i = 0; i < m->n; i++) {
	B[i] = a[i] / M(m, i, i);
    }

    return 1;
}

int main(int argc, char *argv[])
{
    unsigned int r, c, rows, cols, n_valid;	/*  totals  */
    int *mapx_fd, mapy_fd, mapres_fd, mapest_fd;
    int i, j, k, n_predictors;
    double *sumX, sumY, *sumsqX, sumsqY, *sumXY;
    double *meanX, meanY, *varX, varY, *sdX, sdY;
    double yest, yres;       /* estimated y, residual */
    double sumYest, *SSerr_without;
    double SE;
    double meanYest, meanYres, varYest, varYres, sdYest, sdYres;
    double SStot, SSerr, SSreg;
    double **a;
    struct MATRIX *m, *m_all;
    double **B, Rsq, Rsqadj, F, t, AIC, AICc, BIC;
    unsigned int count = 0;
    DCELL **mapx_buf, *mapy_buf, *mapx_val, mapy_val, *mapres_buf, *mapest_buf;
    char *name;
    struct Option *input_mapx, *input_mapy, *output_res, *output_est, *output_opt;
    struct Flag *shell_style;
    struct Cell_head region;
    struct GModule *module;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    module->description =
	_("Calculates multiple linear regression from raster maps.");

    /* Define the different options */
    input_mapx = G_define_standard_option(G_OPT_R_INPUTS);
    input_mapx->key = "mapx";
    input_mapx->description = (_("Map for x coefficient"));

    input_mapy = G_define_standard_option(G_OPT_R_INPUT);
    input_mapy->key = "mapy";
    input_mapy->description = (_("Map for y coefficient"));

    output_res = G_define_standard_option(G_OPT_R_OUTPUT);
    output_res->key = "residuals";
    output_res->required = NO;
    output_res->description = (_("Map to store residuals"));

    output_est = G_define_standard_option(G_OPT_R_OUTPUT);
    output_est->key = "estimates";
    output_est->required = NO;
    output_est->description = (_("Map to store estimates"));

    output_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    output_opt->key = "output";
    output_opt->required = NO;
    output_opt->description =
	(_("ASCII file for storing regression coefficients (output to screen if file not specified)."));

    shell_style = G_define_flag();
    shell_style->key = 'g';
    shell_style->description = _("Print in shell script style");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    name = output_opt->answer;
    if (name != NULL && strcmp(name, "-") != 0) {
	if (NULL == freopen(name, "w", stdout)) {
	    G_fatal_error(_("Unable to open file <%s> for writing"), name);
	}
    }

    G_get_window(&region);
    rows = region.rows;
    cols = region.cols;

    /* count x maps */
    for (i = 0; input_mapx->answers[i]; i++);
    n_predictors = i;
    
    /* allocate memory for x maps */
    mapx_fd = (int *)G_malloc(n_predictors * sizeof(int));
    sumX = (double *)G_malloc(n_predictors * sizeof(double));
    sumsqX = (double *)G_malloc(n_predictors * sizeof(double));
    sumXY = (double *)G_malloc(n_predictors * sizeof(double));
    SSerr_without = (double *)G_malloc(n_predictors * sizeof(double));
    meanX = (double *)G_malloc(n_predictors * sizeof(double));
    varX = (double *)G_malloc(n_predictors * sizeof(double));
    sdX = (double *)G_malloc(n_predictors * sizeof(double));
    mapx_buf = (DCELL **)G_malloc(n_predictors * sizeof(DCELL *));
    mapx_val = (DCELL *)G_malloc((n_predictors + 1) * sizeof(DCELL));
    
    /* ordinary least squares */
    m = NULL;
    m_all = (struct MATRIX *)G_malloc((n_predictors + 1) * sizeof(struct MATRIX));
    a = (double **)G_malloc((n_predictors + 1) * sizeof(double *));
    B = (double **)G_malloc((n_predictors + 1) * sizeof(double *));

    m = &(m_all[0]);
    m->n = n_predictors + 1;
    m->v = (double *)G_malloc(m->n * m->n * sizeof(double));

    a[0] = (double *)G_malloc(m->n * sizeof(double));
    B[0] = (double *)G_malloc(m->n * sizeof(double));

    for (i = 0; i < m->n; i++) {
	for (j = i; j < m->n; j++)
	    M(m, i, j) = 0.0;
	a[0][i] = 0.0;
	B[0][i] = 0.0;
    }
    
    for (k = 1; k <= n_predictors; k++) {
	m = &(m_all[k]);
	m->n = n_predictors;
	m->v = (double *)G_malloc(m->n * m->n * sizeof(double));
	a[k] = (double *)G_malloc(m->n * sizeof(double));
	B[k] = (double *)G_malloc(m->n * sizeof(double));

	for (i = 0; i < m->n; i++) {
	    for (j = i; j < m->n; j++)
		M(m, i, j) = 0.0;
	    a[k][i] = 0.0;
	    B[k][i] = 0.0;
	}
    }

    /* open maps */
    G_debug(1, "open maps");
    for (i = 0; i < n_predictors; i++) {
	mapx_fd[i] = Rast_open_old(input_mapx->answers[i], "");
    }
    mapy_fd = Rast_open_old(input_mapy->answer, "");

    for (i = 0; i < n_predictors; i++)
	mapx_buf[i] = Rast_allocate_d_buf();
    mapy_buf = Rast_allocate_d_buf();

    for (i = 0; i < n_predictors; i++) {
	sumX[i] = sumsqX[i] = sumXY[i] = 0.0;
	meanX[i] = varX[i] = sdX[i] = 0.0;
	SSerr_without[i] = 0.0;
    }
    sumY = sumsqY = meanY = varY = sdY = 0.0;
    sumYest = meanYest = varYest = sdYest = 0.0;
    meanYres = varYres = sdYres = 0.0;

    /* read input maps */
    G_message(_("First pass..."));
    n_valid = 0;
    mapx_val[0] = 1.0;
    for (r = 0; r < rows; r++) {
	G_percent(r, rows, 2);

	for (i = 0; i < n_predictors; i++)
	    Rast_get_d_row(mapx_fd[i], mapx_buf[i], r);

	Rast_get_d_row(mapy_fd, mapy_buf, r);

	for (c = 0; c < cols; c++) {
	    int isnull = 0;

	    for (i = 0; i < n_predictors; i++) {
		mapx_val[i + 1] = mapx_buf[i][c];
		if (Rast_is_d_null_value(&(mapx_val[i + 1]))) {
		    isnull = 1;
		    break;
		}
	    }
	    if (isnull)
		continue;

	    mapy_val = mapy_buf[c];
	    if (Rast_is_d_null_value(&mapy_val))
		continue;

	    for (i = 0; i <= n_predictors; i++) {
		double val1 = mapx_val[i];

		for (j = i; j <= n_predictors; j++) {
		    double val2 = mapx_val[j];

		    m = &(m_all[0]);
		    M(m, i, j) += val1 * val2;

		    /* linear model without predictor k */
		    for (k = 1; k <= n_predictors; k++) {
			if (k != i && k != j) {
			    int i2 = k > i ? i : i - 1;
			    int j2 = k > j ? j : j - 1;

			    m = &(m_all[k]);
			    M(m, i2, j2) += val1 * val2;
			}
		    }
		}

		a[0][i] += mapy_val * val1;
		for (k = 1; k <= n_predictors; k++) {
		    if (k != i) {
			int i2 = k > i ? i : i - 1;

			a[k][i2] += mapy_val * val1;
		    }
		}

		if (i > 0) {
		    sumX[i - 1] += val1;
		    sumsqX[i - 1] += val1 * val1;
		    sumXY[i - 1] += val1 * mapy_val;
		}
	    }

	    sumY += mapy_val;
	    sumsqY += mapy_val * mapy_val;
	    count++;
	}
    }
    G_percent(rows, rows, 2);
    
    if (count < n_predictors + 1)
	G_fatal_error(_("Not enough valid cells available"));

    for (k = 0; k <= n_predictors; k++) {
	m = &(m_all[k]);

	/* TRANSPOSE VALUES IN UPPER HALF OF M TO OTHER HALF */
	for (i = 1; i < m->n; i++)
	    for (j = 0; j < i; j++)
		M(m, i, j) = M(m, j, i);

	if (!solvemat(m, a[k], B[k])) {
	    for (i = 0; i <= n_predictors; i++) {
		fprintf(stdout, "b%d=0.0\n", i);
	    }
	    G_fatal_error(_("Multiple regression failed"));
	}
    }
    
    /* second pass */
    G_message(_("Second pass..."));

    /* residuals output */
    if (output_res->answer) {
	mapres_fd = Rast_open_new(output_res->answer, DCELL_TYPE);
	mapres_buf = Rast_allocate_d_buf();
    }
    else {
	mapres_fd = -1;
	mapres_buf = NULL;
    }

    /* estimates output */
    if (output_est->answer) {
	mapest_fd = Rast_open_new(output_est->answer, DCELL_TYPE);
	mapest_buf = Rast_allocate_d_buf();
    }
    else {
	mapest_fd = -1;
	mapest_buf = NULL;
    }

    for (i = 0; i < n_predictors; i++)
	meanX[i] = sumX[i] / count;

    meanY = sumY / count;
    SStot = SSerr = SSreg = 0.0;
    for (r = 0; r < rows; r++) {
	G_percent(r, rows, 2);

	for (i = 0; i < n_predictors; i++)
	    Rast_get_d_row(mapx_fd[i], mapx_buf[i], r);

	Rast_get_d_row(mapy_fd, mapy_buf, r);
	
	if (mapres_buf)
	    Rast_set_d_null_value(mapres_buf, cols);
	if (mapest_buf)
	    Rast_set_d_null_value(mapest_buf, cols);

	for (c = 0; c < cols; c++) {
	    int isnull = 0;

	    for (i = 0; i < n_predictors; i++) {
		mapx_val[i + 1] = mapx_buf[i][c];
		if (Rast_is_d_null_value(&(mapx_val[i + 1]))) {
		    isnull = 1;
		    break;
		}
	    }
	    if (isnull)
		continue;

	    yest = 0.0;
	    for (i = 0; i <= n_predictors; i++) {
		yest += B[0][i] * mapx_val[i];
	    }
	    if (mapest_buf)
		mapest_buf[c] = yest;

	    mapy_val = mapy_buf[c];
	    if (Rast_is_d_null_value(&mapy_val))
		continue;

	    yres = mapy_val - yest;
	    if (mapres_buf)
		mapres_buf[c] = yres;

	    SStot += (mapy_val - meanY) * (mapy_val - meanY);
	    SSreg += (yest - meanY) * (yest - meanY);
	    SSerr += yres * yres;

	    for (k = 1; k <= n_predictors; k++) {
		double yesti = 0.0;
		double yresi;

		/* linear model without predictor k */
		for (i = 0; i <= n_predictors; i++) {
		    if (i != k) {
			j = k > i ? i : i - 1;
			yesti += B[k][j] * mapx_val[i];
		    }
		}
		yresi = mapy_val - yesti;

		/* linear model without predictor k */
		SSerr_without[k - 1] += yresi * yresi;

		varX[k - 1] = (mapx_val[k] - meanX[k - 1]) * (mapx_val[k] - meanX[k - 1]);
	    }
	}

	if (mapres_buf)
	    Rast_put_d_row(mapres_fd, mapres_buf);
	if (mapest_buf)
	    Rast_put_d_row(mapest_fd, mapest_buf);
    }
    G_percent(rows, rows, 2);

    fprintf(stdout, "n=%d\n", count);
    /* coefficient of determination aka R squared */
    Rsq = 1 - (SSerr / SStot);
    fprintf(stdout, "Rsq=%f\n", Rsq);
    /* adjusted coefficient of determination */
    Rsqadj = 1 - ((SSerr * (count - 1)) / (SStot * (count - n_predictors - 1)));
    fprintf(stdout, "Rsqadj=%f\n", Rsqadj);
    /* F statistic */
    /* F = ((SStot - SSerr) / (n_predictors)) / (SSerr / (count - n_predictors));
     * , or: */
    F = ((SStot - SSerr) * (count - n_predictors - 1)) / (SSerr * (n_predictors));
    fprintf(stdout, "F=%f\n", F);

    i = 0;
    /* constant aka estimate for intercept in R */
    fprintf(stdout, "b%d=%f\n", i, B[0][i]);
    /* t score for R squared of the full model, unused */
    t = sqrt(Rsq) * sqrt((count - 2) / (1 - Rsq));
    /*
    fprintf(stdout, "t%d=%f\n", i, t);
    */

    /* AIC, corrected AIC, and BIC information criteria for the full model */
    AIC = count * log(SSerr / count) + 2 * (n_predictors + 1);
    fprintf(stdout, "AIC=%f\n", AIC);
    AICc = AIC + (2 * n_predictors * (n_predictors + 1)) / (count - n_predictors - 1);
    fprintf(stdout, "AICc=%f\n", AICc);
    BIC = count * log(SSerr / count) + log(count) * (n_predictors + 1);
    fprintf(stdout, "BIC=%f\n", BIC);

    /* error variance of the model, identical to R */
    SE = SSerr / (count - n_predictors - 1);
    /*
    fprintf(stdout, "SE=%f\n", SE);
    fprintf(stdout, "SSerr=%f\n", SSerr);
    */

    for (i = 0; i < n_predictors; i++) {

	fprintf(stdout, "\nb%d=%f\n", i + 1, B[0][i + 1]);
	if (n_predictors > 1) {
	    double Rsqi, SEi, sumsqX_corr;

	    /* corrected sum of squares for predictor [i] */
	    sumsqX_corr = sumsqX[i] - sumX[i] * sumX[i] / (count - n_predictors - 1);

	    /* standard error SE for predictor [i] */

	    /* SE[i] with only one predictor: sqrt(SE / sumsqX_corr)
	     * this does not work with more than one predictor */
	    /* in R, SEi is sqrt(diag(R) * resvar) with
	     * R = ???
	     * resvar = rss / rdf = SE global
	     * rss = sum of squares of the residuals
	     * rdf = residual degrees of freedom = count - n_predictors - 1 */
	    SEi = sqrt(SE / (Rsq * sumsqX_corr));
	    /*
	    fprintf(stdout, "SE%d=%f\n", i + 1, SEi);
	    */

	    /* Sum of squares for predictor [i] */
	    /*
	    fprintf(stdout, "SSerr%d=%f\n", i + 1, SSerr_without[i] - SSerr);
	    */

	    /* R squared of the model without predictor [i] */
	    /* Rsqi = 1 - SSerr_without[i] / SStot; */
	    /* the additional amount of variance explained
	     * when including predictor [i] :
	     * Rsq - Rsqi */
	    Rsqi = (SSerr_without[i] - SSerr) / SStot;
	    fprintf(stdout, "Rsq%d=%f\n", i + 1, Rsqi);

	    /* t score for Student's t distribution, unused */
	    t = (B[0][i + 1]) / SEi;
	    /*
	    fprintf(stdout, "t%d=%f\n", i + 1, t);
	    */

	    /* F score for Fisher's F distribution
	     * here: F score to test if including predictor [i]
	     * yields a significant improvement
	     * after Lothar Sachs, Angewandte Statistik:
	     * F = (Rsq - Rsqi) * (count - n_predictors - 1) / (1 - Rsq) */
	    /* same like Sumsq / SE */
	    /* same like (SSerr_without[i] / SSerr - 1) * (count - n_predictors - 1) */
	    /* same like R-stats when entered in R-stats as last predictor */
	    F = (SSerr_without[i] / SSerr - 1) * (count - n_predictors - 1);
	    fprintf(stdout, "F%d=%f\n", i + 1, F);

	    /* AIC, corrected AIC, and BIC information criteria for
	     * the model without predictor [i] */
	    AIC = count * log(SSerr_without[i] / count) + 2 * (n_predictors);
	    fprintf(stdout, "AIC%d=%f\n", i + 1, AIC);
	    AICc = AIC + (2 * (n_predictors - 1) * n_predictors) / (count - n_predictors - 2);
	    fprintf(stdout, "AICc%d=%f\n", i + 1, AICc);
	    BIC = count * log(SSerr_without[i] / count) + (n_predictors - 1) * log(count);
	    fprintf(stdout, "BIC%d=%f\n", i + 1, BIC);
	}
    }
    

    for (i = 0; i < n_predictors; i++) {
	Rast_close(mapx_fd[i]);
	G_free(mapx_buf[i]);
    }
    Rast_close(mapy_fd);
    G_free(mapy_buf);
    
    if (mapres_fd > -1) {
	struct History history;

	Rast_close(mapres_fd);
	G_free(mapres_buf);

	Rast_short_history(output_res->answer, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(output_res->answer, &history);
    }

    if (mapest_fd > -1) {
	struct History history;

	Rast_close(mapest_fd);
	G_free(mapest_buf);

	Rast_short_history(output_est->answer, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(output_est->answer, &history);
    }

    exit(EXIT_SUCCESS);
}
