#include <stdlib.h>
#include <limits.h>

#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/glocale.h>

#include "local_proto.h"

int transform(int *datafds, int *outfds, int rows, int cols, double **eigmat,
              int bands, CELL *mins, CELL *maxs)
{
    int i, j, k, l;
    double *sum;
    DCELL **rowbufs;

    sum = G_alloc_vector(bands);
    rowbufs = (DCELL **)G_calloc(bands, sizeof(DCELL *));

    /* allocate row buffers for each band */
    for (i = 0; i < bands; i++) {
        rowbufs[i] = Rast_allocate_d_buf();
        if (!rowbufs[i])
            G_fatal_error(_("Unable to allocate DCELL buffers."));
        mins[i] = INT_MAX;
        maxs[i] = INT_MIN;
    }

    for (i = 0; i < rows; i++) {
        /* read each input band as CELL, convert to DCELL */
        for (j = 0; j < bands; j++) {
            CELL *tmp = Rast_allocate_c_buf();
            Rast_get_c_row(datafds[j], tmp, i);
            for (l = 0; l < cols; l++)
                rowbufs[j][l] = (DCELL)tmp[l];
            G_free(tmp);
        }

        /* apply canonical transform */
        for (l = 0; l < cols; l++) {
            for (j = 0; j < bands; j++) {
                sum[j] = 0.0;
                for (k = 0; k < bands; k++) {
                    sum[j] += eigmat[j][k] * rowbufs[k][l];
                }
            }
            for (j = 0; j < bands; j++) {
                rowbufs[j][l] = sum[j];
                /*min/max are tracked as integer CELL */
                if ((CELL)rowbufs[j][l] > maxs[j])
                    maxs[j] = (CELL)rowbufs[j][l];
                if ((CELL)rowbufs[j][l] < mins[j])
                    mins[j] = (CELL)rowbufs[j][l];
            }
        }

        /* write each output band row as DCELL */
        for (j = 0; j < bands; j++)
            Rast_put_d_row(outfds[j], rowbufs[j]);
    }

    for (i = 0; i < bands; i++)
        G_free(rowbufs[i]);

    G_free(rowbufs);
    G_free_vector(sum);

    G_message(_("Canonical transform completed."));

    return 0;
}
