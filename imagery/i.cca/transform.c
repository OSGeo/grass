#include <stdlib.h>

#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/glocale.h>

#include "local_proto.h"

int
transform(int *datafds, int *outfds, int rows, int cols,
	  double **eigmat, int bands, CELL *mins, CELL *maxs)
{
    int i, j, k, l;
    double *sum;
    CELL **rowbufs;

    sum = G_alloc_vector(bands);
    rowbufs = (CELL**)G_calloc(bands, sizeof(CELL*));


    /* allocate row buffers for each band */
    for (i = 0; i < bands; i++)
	if ((rowbufs[i] = Rast_allocate_c_buf()) == NULL)
	    G_fatal_error(_("Unable to allocate cell buffers."));

    for (i = 0; i < rows; i++) {
	/* get one row of data */
	for (j = 0; j < bands; j++)
	    Rast_get_c_row(datafds[j], rowbufs[j], i);

	/* transform each cell in the row */
	for (l = 0; l < cols; l++) {
	    for (j = 0; j < bands; j++) {
		sum[j] = 0.0;
		for (k = 0; k < bands; k++) {
		    sum[j] += eigmat[j][k] * (double)rowbufs[k][l];
		}
	    }
	    for (j = 0; j < bands; j++) {
		rowbufs[j][l] = (CELL) (sum[j] + 0.5);
		if (rowbufs[j][l] > maxs[j])
		    maxs[j] = rowbufs[j][l];
		if (rowbufs[j][l] < mins[j])
		    mins[j] = rowbufs[j][l];
	    }
	}

	/* output the row of data */
	for (j = 0; j < bands; j++)
	    Rast_put_row(outfds[j], rowbufs[j], CELL_TYPE);
    }
    for (i = 0; i < bands; i++)
	G_free(rowbufs[i]);

    G_free(rowbufs);
    G_free_vector(sum);

    G_message(_("Transform completed.\n"));

    return 0;
}
