#include <stdlib.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "files.h"
/* #include "local_proto.h" */

int read_data(struct files *files, struct SigSet *S)
{
    int n;
    int b;
    int nrows, ncols, row, col;
    CELL *class;
    struct ClassData *Data;

    nrows = G_window_rows();
    ncols = G_window_cols();
    class = (CELL *) G_calloc(ncols, sizeof(CELL));

    G_message(_("Reading raster maps..."));

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	read_training_map(class, row, ncols, files);
	for (b = 0; b < files->nbands; b++)
	    if (G_get_d_raster_row
		(files->band_fd[b], files->band_cell[b], row) < 0)
		G_fatal_error(_("Unable to read raster map row %d"),
			      row);

	for (col = 0; col < ncols; col++) {
	    n = class[col];
	    if (n < 0)
		continue;
	    Data = &S->ClassSig[n].ClassData;
	    for (b = 0; b < files->nbands; b++) {
		if (G_is_d_null_value(&files->band_cell[b][col]))
		    G_set_d_null_value(&Data->x[Data->count][b], 1);
		else
		    Data->x[Data->count][b] = files->band_cell[b][col];
	    }
	    Data->count++;
	}
    }
    G_percent(nrows, nrows, 2);
    G_free(class);

    return 0;
}
