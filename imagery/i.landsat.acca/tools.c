#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"


/*--------------------------------------------------------
  HISTOGRAM ANALYSIS
  Define un factor de escala = hist_n/100 con objeto
  de dividir el entero 1 por 100/hist_n partes y
  aumentar la precision.

  Afecta al almacenamiento en el histograma pero
  modifica el calculo de quantiles y momentos.
 --------------------------------------------------------*/

/* Global variable
   allow use as parameter in the command line */
int hist_n = 100;		/* interval of real data 100/hist_n */

void hist_put(double t, int hist[])
{
    int i;

    /* scale factor */
    i = (int)(t * ((double)hist_n / 100.));

    if (i < 1)
	i = 1;
    if (i > hist_n)
	i = hist_n;

    hist[i - 1] += 1;
}

/* histogram moment */
double moment(int n, int hist[], int k)
{
    int i, total;
    double value, mean;

    k = 0;

    total = 0;
    mean = 0.;
    for (i = 0; i < hist_n; i++) {
	total += hist[i];
	mean += (double)(i * hist[i]);
    }
    mean /= ((double)total);	/* histogram mean */

    value = 0.;
    for (i = 0; i < hist_n; i++) {
	value += (pow((i - mean), n) * ((double)hist[i]));
    }
    value /= (double)(total - k);

    return (value / pow((double)hist_n / 100., n) );
}

/* Real data quantile */
double quantile(double q, int hist[])
{
    int i, total;
    double value, qmax, qmin;

    total = 0;
    for (i = 0; i < hist_n; i++) {
	total += hist[i];
    }

    value = 0;
    qmax = 1.;
    for (i = hist_n - 1; i >= 0; i--) {
	qmin = qmax - (double)hist[i] / (double)total;
	if (q >= qmin) {
	    value = (q - qmin) / (qmax - qmin) + (i - 1);
	    break;
	}
	qmax = qmin;
    }

    /* remove scale factor */
    return (value / ((double)hist_n / 100.));
}

/*--------------------------------------------------------
    FILTER HOLES OF CLOUDS
    This a >=50% filter of 3x3
    if >= 50% vecinos cloud then pixel set to cloud
 --------------------------------------------------------*/

int pval(void *rast, int i)
{
    void *ptr = (void *)((CELL *) rast + i);

    if (Rast_is_c_null_value(ptr))
	return 0;
    else
	return (int)((CELL *) rast)[i];
}

void filter_holes(Gfile * out)
{
    int row, col, nrows, ncols;

    void *arast, *brast, *crast;
    int i, pixel[9], cold, warm, shadow, nulo, lim;

    Gfile tmp;

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    if (nrows < 3 || ncols < 3)
	return;

    /* Open to read */
    if ((out->fd = Rast_open_old(out->name, "")) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), out->name);
    
    arast = Rast_allocate_buf(CELL_TYPE);
    brast = Rast_allocate_buf(CELL_TYPE);
    crast = Rast_allocate_buf(CELL_TYPE);

    /* Open to write */
    sprintf(tmp.name, "_%d.BBB", getpid());
    tmp.rast = Rast_allocate_buf(CELL_TYPE);
    if ((tmp.fd = Rast_open_new(tmp.name, CELL_TYPE)) < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), tmp.name);

    G_important_message(_("Filling small holes in clouds..."));

    /* Se puede acelerar creandolos nulos y luego arast = brast
       brast = crast y cargando crast solamente
       G_set_f_null_value(cell[2], ncols);
     */

    for (row = 0; row < nrows; row++) {
      	G_percent(row, nrows, 2);
	/* Read row values */
	if (row != 0) {
	    Rast_get_c_row(out->fd, arast, row - 1);
	}
	Rast_get_c_row(out->fd, brast, row);
	if (row != (nrows - 1)) {
	    Rast_get_c_row(out->fd, crast, row + 1);
	}
	/* Analysis of all pixels */
	for (col = 0; col < ncols; col++) {
	    pixel[0] = pval(brast, col);
	    if (pixel[0] == 0) {
		if (row == 0) {
		    pixel[1] = -1;
		    pixel[2] = -1;
		    pixel[3] = -1;
		    if (col == 0) {
			pixel[4] = -1;
			pixel[5] = pval(brast, col + 1);
			pixel[6] = -1;
			pixel[7] = pval(crast, col);
			pixel[8] = pval(crast, col + 1);
		    }
		    else if (col != (ncols - 1)) {
			pixel[4] = pval(brast, col - 1);
			pixel[5] = pval(brast, col + 1);
			pixel[6] = pval(crast, col - 1);
			pixel[7] = pval(crast, col);
			pixel[8] = pval(crast, col + 1);
		    }
		    else {
			pixel[4] = pval(brast, col - 1);
			pixel[5] = -1;
			pixel[6] = pval(crast, col - 1);
			pixel[7] = pval(crast, col);
			pixel[8] = -1;
		    }
		}
		else if (row != (nrows - 1)) {
		    if (col == 0) {
			pixel[1] = -1;
			pixel[2] = pval(arast, col);
			pixel[3] = pval(arast, col + 1);
			pixel[4] = -1;
			pixel[5] = pval(brast, col + 1);
			pixel[6] = -1;
			pixel[7] = pval(crast, col);
			pixel[8] = pval(crast, col + 1);
		    }
		    else if (col != (ncols - 1)) {
			pixel[1] = pval(arast, col - 1);
			pixel[2] = pval(arast, col);
			pixel[3] = pval(arast, col + 1);
			pixel[4] = pval(brast, col - 1);
			pixel[5] = pval(brast, col + 1);
			pixel[6] = pval(crast, col - 1);
			pixel[7] = pval(crast, col);
			pixel[8] = pval(crast, col + 1);
		    }
		    else {
			pixel[1] = pval(arast, col - 1);
			pixel[2] = pval(arast, col);
			pixel[3] = -1;
			pixel[4] = pval(brast, col - 1);
			pixel[5] = -1;
			pixel[6] = pval(crast, col - 1);
			pixel[7] = pval(crast, col);
			pixel[8] = -1;
		    }
		}
		else {
		    pixel[6] = -1;
		    pixel[7] = -1;
		    pixel[8] = -1;
		    if (col == 0) {
			pixel[1] = -1;
			pixel[2] = pval(arast, col);
			pixel[3] = pval(arast, col + 1);
			pixel[4] = -1;
			pixel[5] = pval(brast, col + 1);
		    }
		    else if (col != (ncols - 1)) {
			pixel[1] = pval(arast, col - 1);
			pixel[2] = pval(arast, col);
			pixel[3] = pval(arast, col + 1);
			pixel[4] = pval(brast, col - 1);
			pixel[5] = pval(brast, col + 1);
		    }
		    else {
			pixel[1] = pval(arast, col - 1);
			pixel[2] = pval(arast, col);
			pixel[3] = -1;
			pixel[4] = pval(brast, col - 1);
			pixel[5] = -1;
		    }
		}

		cold = warm = shadow = nulo = 0;
		for (i = 1; i < 9; i++) {
		    switch (pixel[i]) {
		    case IS_COLD_CLOUD:
			cold++;
			break;
		    case IS_WARM_CLOUD:
			warm++;
			break;
		    case IS_SHADOW:
			shadow++;
			break;
		    default:
			nulo++;
			break;
		    }
		}
		lim = (int)(cold + warm + shadow + nulo) / 2;

		/* Entra pixel[0] = 0 */
		if (nulo < lim) {
		    if (shadow >= (cold + warm))
			pixel[0] = IS_SHADOW;
		    else
			pixel[0] =
			    (warm > cold) ? IS_WARM_CLOUD : IS_COLD_CLOUD;
		}
	    }
	    if (pixel[0] != 0) {
		((CELL *) tmp.rast)[col] = pixel[0];
	    }
	    else {
		Rast_set_c_null_value((CELL *) tmp.rast + col, 1);
	    }
	}
	Rast_put_row(tmp.fd, tmp.rast, CELL_TYPE);
    }
    G_percent(1, 1, 1);
    
    G_free(arast);
    G_free(brast);
    G_free(crast);
    Rast_close(out->fd);

    G_free(tmp.rast);
    Rast_close(tmp.fd);

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
