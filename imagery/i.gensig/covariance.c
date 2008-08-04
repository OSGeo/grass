#include <stdlib.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "signature.h"
#include "files.h"
#include "local_proto.h"


/* must be called after compute_means() */
int compute_covariances(struct files *files, struct Signature *S)
{
    int n;
    int b, b1, b2;
    int nrows, ncols, row, col;
    CELL *class;
    DCELL *cell1, *cell2;

    for (n = 0; n < S->nsigs; n++)	/* for each signature (aka class) */
	for (b1 = 0; b1 < S->nbands; b1++)
	    for (b2 = 0; b2 < S->nbands; b2++)
		S->sig[n].var[b1][b2] = 0.0;

    nrows = G_window_rows();
    ncols = G_window_cols();
    class = (CELL *) G_calloc(ncols, sizeof(CELL));

    if (S->nsigs == 1)
	G_message(_("Calculating class covariance matrix..."));
    else
	G_message(_("Calculating class covariance matrices..."));

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	read_training_map(class, row, ncols, files);
	for (b = 0; b < files->nbands; b++)	/* NOTE: files->nbands == S->nbands */
	    if (G_get_d_raster_row
		(files->band_fd[b], files->band_cell[b], row) < 0)
		exit(1);
	for (b1 = 0; b1 < files->nbands; b1++) {
	    cell1 = files->band_cell[b1];
	    for (b2 = 0; b2 <= b1; b2++) {	/* only need to calculate the lower half */
		cell2 = files->band_cell[b2];
		for (col = 0; col < ncols; col++) {
		    n = class[col];
		    if (n < 0)
			continue;
		    S->sig[n].var[b1][b2] +=
			(cell1[col] - S->sig[n].mean[b1]) * (cell2[col] -
							     S->sig[n].
							     mean[b2]);
		}
	    }
	}
    }
    G_percent(row, nrows, 2);
    for (n = 0; n < S->nsigs; n++)	/* for each signature (aka class) */
	for (b1 = 0; b1 < S->nbands; b1++)
	    for (b2 = 0; b2 <= b1; b2++) {
		S->sig[n].var[b1][b2] /= (S->sig[n].npoints - 1);
		if (b1 != b2)
		    S->sig[n].var[b2][b1] = S->sig[n].var[b1][b2];
	    }
    G_free(class);

    return 0;
}
