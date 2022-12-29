/* File: algorithm.c
 *
 *  AUTHOR:    E. Jorge Tizado, Spain 2010
 *
 *  COPYRIGHT: (c) 2010 E. Jorge Tizado
 *             This program is free software under the GNU General Public
 *             License (>=v2). Read the file COPYING that comes with GRASS
 *             for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

#define SCALE   200.
#define K_BASE  230.


/* value and count */
#define TOTAL 0
#define WARM  1
#define COLD  2
#define SNOW  3
#define SOIL  4

/* signa */
#define COVER       1
#define SUM_COLD    0
#define SUM_WARM    1
#define KMEAN       2
#define KMAX        3
#define KMIN        4

/* re-use value */
#define KLOWER      0
#define KUPPER      1
#define MEAN        2
#define SKEW        3
#define DSTD        4


/**********************************************************
 *
 * Automatic Cloud Cover Assessment (ACCA): Irish 2000
 *
 **********************************************************/

/*--------------------------------------------------------
  CONSTANTS
  Usar esta forma para que via extern puedan modificarse
  como opciones desde el programa main.
 ---------------------------------------------------------*/

double th_1 = 0.08;		/* Band 3 Brightness Threshold */
double th_1_b = 0.07;
double th_2[] = { -0.25, 0.70 };	/* Normalized Snow Difference Index */

double th_2_b = 0.8;
double th_3 = 300.;		/* Band 6 Temperature Threshold */
double th_4 = 225.;		/* Band 5/6 Composite */
double th_4_b = 0.08;
double th_5 = 2.35;		/* Band 4/3 Ratio */
double th_6 = 2.16248;		/* Band 4/2 Ratio */
double th_7 = 1.0; /* Band 4/5 Ratio */ ;
double th_8 = 210.;		/* Band 5/6 Composite */

extern int hist_n;

void acca_algorithm(Gfile * out, Gfile band[],
		    int single_pass, int with_shadow, int cloud_signature)
{
    int i, count[5], hist_cold[hist_n], hist_warm[hist_n];
    double max, value[5], signa[5], idesert, review_warm, shift;

    /* Reset variables ... */
    for (i = 0; i < 5; i++) {
	count[i] = 0;
	value[i] = 0.;
    }
    for (i = 0; i < hist_n; i++) {
	hist_cold[i] = hist_warm[i] = 0;
    }

    /* FIRST FILTER ... */
    acca_first(out, band, with_shadow,
	       count, hist_cold, hist_warm, signa);
    /* CATEGORIES: NO_DEFINED, WARM_CLOUD, COLD_CLOUD, NULL (= NO_CLOUD) */

    value[WARM] = (double)count[WARM] / (double)count[TOTAL];
    value[COLD] = (double)count[COLD] / (double)count[TOTAL];
    value[SNOW] = (double)count[SNOW] / (double)count[TOTAL];
    value[SOIL] = (double)count[SOIL] / (double)count[TOTAL];

    value[0] = (double)(count[WARM] + count[COLD]);
    idesert = (value[0] == 0. ? 0. : value[0] / ((double)count[SOIL]));

    /* BAND-6 CLOUD SIGNATURE DEVELOPMENT */
    if (idesert <= .5 || value[SNOW] > 0.01) {
	/* Only the cold clouds are used
	   if snow or desert soil is present */
	review_warm = 1;
    }
    else {
	/* The cold and warm clouds are combined
	   and treated as a single population */
	review_warm = 0;
	count[COLD] += count[WARM];
	value[COLD] += value[WARM];
	signa[SUM_COLD] += signa[SUM_WARM];
	for (i = 0; i < hist_n; i++)
	    hist_cold[i] += hist_warm[i];
    }

    signa[KMEAN] = SCALE * signa[SUM_COLD] / ((double)count[COLD]);
    signa[COVER] = ((double)count[COLD]) / ((double)count[TOTAL]);

    G_message(_("Preliminary scene analysis:"));
    G_message(_("* Desert index: %.2lf"), idesert);
    G_message(_("* Snow cover: %.2lf %%"), 100. * value[SNOW]);
    G_message(_("* Cloud cover: %.2lf %%"), 100. * signa[COVER]);
    G_message(_("* Temperature of clouds:"));
    G_message(_("** Maximum: %.2lf K"), signa[KMAX]);
    G_message(_("** Mean (%s cloud): %.2lf K"),
	    (review_warm ? "cold" : "all"), signa[KMEAN]);
    G_message(_("** Minimum: %.2lf K"), signa[KMIN]);

    /* WARNING: re-use of the variable 'value' with new meaning */

    /* step 14 */

    /* To correct Irish2006: idesert has to be bigger than 0.5 to start pass 2 processing (see Irish2000)
       because then we have no desert condition (thanks to Matthias Eder, Germany) */
    if (cloud_signature ||
	(idesert > .5 && signa[COVER] > 0.004 && signa[KMEAN] < 295.)) {
	G_message(_("Histogram cloud signature:"));

	value[MEAN] = quantile(0.5, hist_cold) + K_BASE;
	value[DSTD] = sqrt(moment(2, hist_cold, 1));
	value[SKEW] = moment(3, hist_cold, 3) / pow(value[DSTD], 3);

	G_message(_("* Mean temperature: %.2lf K"), value[MEAN]);
	G_message(_("* Standard deviation: %.2lf"), value[DSTD]);
	G_message(_("* Skewness: %.2lf"), value[SKEW]);
	G_message(_("* Histogram classes: %d"), hist_n);

	shift = value[SKEW];
	if (shift > 1.)
	    shift = 1.;
	else if (shift < 0.)
	    shift = 0.;

	max = quantile(0.9875, hist_cold) + K_BASE;
	value[KUPPER] = quantile(0.975, hist_cold) + K_BASE;
	value[KLOWER] = quantile(0.835, hist_cold) + K_BASE;

	G_message(_("* 98.75 percentile: %.2lf K"), max);
	G_message(_("* 97.50 percentile: %.2lf K"), value[KUPPER]);
	G_message(_("* 83.50 percentile: %.2lf K"), value[KLOWER]);

	/* step 17 & 18 */
	if (shift > 0.) {
	    shift *= value[DSTD];

	    if ((value[KUPPER] + shift) > max) {
                if ((value[KLOWER] + shift) > max) {
                    value[KLOWER] += (max - value[KUPPER]);
                }
                else {
                    value[KLOWER] += shift;
                }
		value[KUPPER] = max;
	    }
	    else {
		value[KLOWER] += shift;
		value[KUPPER] += shift;
	    }
	}

	G_message(_("Maximum temperature:"));
	G_message(_("* Cold cloud: %.2lf K"), value[KUPPER]);
	G_message(_("* Warm cloud: %.2lf K"), value[KLOWER]);
    }
    else {
	if (signa[KMEAN] < 295.) {
	    /* Retained warm and cold clouds */
	    G_message(_("Result: Scene with clouds"));
	    review_warm = 0;
	    value[KUPPER] = 0.;
	    value[KLOWER] = 0.;
	}
	else {
	    /* Retained cold clouds */
	    G_message(_("Result: Scene cloud free"));
	    review_warm = 1;
	    value[KUPPER] = 0.;
	    value[KLOWER] = 0.;
	}
    }

    /* SECOND FILTER ... */
    /* By-pass two processing but it retains warm and cold clouds */
    if (single_pass == TRUE) {
	review_warm = -1.;
	value[KUPPER] = 0.;
	value[KLOWER] = 0.;
    }
    acca_second(out, band[BAND6],
		review_warm, value[KUPPER], value[KLOWER]);
    /* CATEGORIES: IS_WARM_CLOUD, IS_COLD_CLOUD, IS_SHADOW, NULL (= NO_CLOUD) */

    return;
}


void acca_first(Gfile *out, Gfile band[],
		int with_shadow,
		int count[], int cold[], int warm[], double stats[])
{
    int i, row, col, nrows, ncols;

    char code;
    double pixel[5], nsdi, rat56;

    /* Creation of output file */
    out->rast = Rast_allocate_buf(CELL_TYPE);
    if ((out->fd = Rast_open_new(out->name, CELL_TYPE)) < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), out->name);

    /* ----- ----- */
    G_important_message(_("Processing first pass..."));

    stats[SUM_COLD] = 0.;
    stats[SUM_WARM] = 0.;
    stats[KMAX] = 0.;
    stats[KMIN] = 10000.;

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	for (i = BAND2; i <= BAND6; i++) {
	    Rast_get_d_row(band[i].fd, band[i].rast, row);
	}
	for (col = 0; col < ncols; col++) {
	    code = NO_DEFINED;
	    /* Null when null pixel in any band */
	    for (i = BAND2; i <= BAND6; i++) {
		if (Rast_is_d_null_value((void *)((DCELL *) band[i].rast + col))) {
		    code = NO_CLOUD;
		    break;
		}
		pixel[i] = (double)((DCELL *) band[i].rast)[col];
	    }
	    /* Determina los pixeles de sombras */
	    if (code == NO_DEFINED && with_shadow) {
		code = shadow_algorithm(pixel);
	    }
	    /* Analiza el valor de los pixeles no definidos */
	    if (code == NO_DEFINED) {
		code = NO_CLOUD;
		count[TOTAL]++;
		nsdi = (pixel[BAND2] - pixel[BAND5]) /
		    (pixel[BAND2] + pixel[BAND5]);
		/* ----------------------------------------------------- */
		/* step 1. Brightness Threshold: Eliminates dark images */
		if (pixel[BAND3] > th_1) {
		    /* step 3. Normalized Snow Difference Index: Eliminates many types of snow */
		    if (nsdi > th_2[0] && nsdi < th_2[1]) {
			/* step 5. Temperature Threshold: Eliminates warm image features */
			if (pixel[BAND6] < th_3) {
			    rat56 = (1. - pixel[BAND5]) * pixel[BAND6];
			    /* step 6. Band 5/6 Composite: Eliminates numerous categories including ice */
			    if (rat56 < th_4) {
				/* step 8. Eliminates growing vegetation */
				if ((pixel[BAND4] / pixel[BAND3]) < th_5) {
				    /* step 9. Eliminates senescing vegetation */
				    if ((pixel[BAND4] / pixel[BAND2]) < th_6) {
					/* step 10. Eliminates rocks and desert */
					count[SOIL]++;
					if ((pixel[BAND4] / pixel[BAND5]) >
					    th_7) {
					    /* step 11. Distinguishes warm clouds from cold clouds */
					    if (rat56 < th_8) {
						code = COLD_CLOUD;
						count[COLD]++;
						/* for statistic */
						stats[SUM_COLD] +=
						    (pixel[BAND6] / SCALE);
						hist_put(pixel[BAND6] -
							 K_BASE, cold);
					    }
					    else {
						code = WARM_CLOUD;
						count[WARM]++;
						/* for statistic */
						stats[SUM_WARM] +=
						    (pixel[BAND6] / SCALE);
						hist_put(pixel[BAND6] -
							 K_BASE, warm);
					    }
					    if (pixel[BAND6] > stats[KMAX])
						stats[KMAX] = pixel[BAND6];
					    if (pixel[BAND6] < stats[KMIN])
						stats[KMIN] = pixel[BAND6];
					}
					else {
					    code = NO_DEFINED;
					}
				    }
				    else {
					code = NO_DEFINED;
					count[SOIL]++;
				    }
				}
				else {
				    code = NO_DEFINED;
				}
			    }
			    else {
				/* step 7 */
				code =
				    (pixel[BAND5] <
				     th_4_b) ? NO_CLOUD : NO_DEFINED;
			    }
			}
			else {
			    code = NO_CLOUD;
			}
		    }
		    else {
			/* step 3 */
			code = NO_CLOUD;
			if (nsdi > th_2_b)
			    count[SNOW]++;
		    }
		}
		else {
		    /* step 2 */
		    code = (pixel[BAND3] < th_1_b) ? NO_CLOUD : NO_DEFINED;
		}
		/* ----------------------------------------------------- */
	    }
	    if (code == NO_CLOUD) {
		Rast_set_c_null_value((CELL *) out->rast + col, 1);
	    }
	    else {
		((CELL *) out->rast)[col] = code;
	    }
	}
	Rast_put_row(out->fd, out->rast, CELL_TYPE);
    }
    G_percent(1, 1, 1);
    
    G_free(out->rast);
    Rast_close(out->fd);

    return;
}


void acca_second(Gfile * out, Gfile band,
		 int review_warm, double upper, double lower)
{
    int row, col, nrows, ncols;

    int code;
    double temp;
    Gfile tmp;

    /* Open to read */
    if ((out->fd = Rast_open_old(out->name, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), out->name);
    
    out->rast = Rast_allocate_buf(CELL_TYPE);
    
    /* Open to write */
    sprintf(tmp.name, "_%d.BBB", getpid());
    tmp.rast = Rast_allocate_buf(CELL_TYPE);
    if ((tmp.fd = Rast_open_new(tmp.name, CELL_TYPE)) < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), tmp.name);

    if (upper == 0.)
	G_important_message(_("Removing ambiguous pixels..."));
    else
	G_important_message(_("Pass two processing..."));

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	
	Rast_get_d_row(band.fd, band.rast, row);
	Rast_get_c_row(out->fd, out->rast, row);
	
	for (col = 0; col < ncols; col++) {
	    if (Rast_is_c_null_value((void *)((CELL *) out->rast + col))) {
		Rast_set_c_null_value((CELL *) tmp.rast + col, 1);
	    }
	    else {
		code = (int)((CELL *) out->rast)[col];
		/* Resolve ambiguous pixels */
		if (code == NO_DEFINED ||
		    (code == WARM_CLOUD && review_warm == 1)) {
		    temp = (double)((DCELL *) band.rast)[col];
		    if (temp > upper) {
			Rast_set_c_null_value((CELL *) tmp.rast + col, 1);
		    }
		    else {
			((CELL *) tmp.rast)[col] =
			    (temp < lower) ? IS_WARM_CLOUD : IS_COLD_CLOUD;
		    }
		}
		else
		    /* Join warm (not ambiguous) and cold clouds */
		if (code == COLD_CLOUD || code == WARM_CLOUD) {
		    ((CELL *) tmp.rast)[col] = (code == WARM_CLOUD &&
						review_warm ==
						0) ? IS_WARM_CLOUD :
			IS_COLD_CLOUD;
		}
		else
		    ((CELL *) tmp.rast)[col] = IS_SHADOW;
	    }
	}
	Rast_put_row(tmp.fd, tmp.rast, CELL_TYPE);
    }
    G_percent(1, 1, 1);
    
    G_free(tmp.rast);
    Rast_close(tmp.fd);

    G_free(out->rast);
    Rast_close(out->fd);

    G_remove("cats", out->name);
    G_remove("cell", out->name);
    G_remove("cellhd", out->name);
    G_remove("cell_misc", out->name);
    G_remove("hist", out->name);

    G_rename("cats", tmp.name, out->name);
    G_rename("cell", tmp.name, out->name);
    G_rename("cellhd", tmp.name, out->name);
    G_rename("cell_misc", tmp.name, out->name);
    G_rename("hist", tmp.name, out->name);

    return;
}

/**********************************************************
 *
 * Cloud shadows
 *
 **********************************************************/

int shadow_algorithm(double pixel[])
{
    /* I think this filter is better but not in any paper */
    if (pixel[BAND3] < 0.07 && (1 - pixel[BAND4]) * pixel[BAND6] > 240. &&
	pixel[BAND4] / pixel[BAND2] > 1. &&
	(pixel[BAND3] - pixel[BAND5]) / (pixel[BAND3] + pixel[BAND5]) < 0.10)
	/*
	   if (pixel[BAND3] < 0.07 && (1 - pixel[BAND4]) * pixel[BAND6] > 240. &&
	   pixel[BAND4] / pixel[BAND2] > 1.)
	 */
    {
	return IS_SHADOW;
    }

    return NO_DEFINED;
}
