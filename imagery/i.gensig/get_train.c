#include <stdlib.h>
#include <string.h>

#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

#include "signature.h"
#include "files.h"


int get_training_classes(struct files *files, struct Signature *S)
{
    int fd;
    CELL *cell;
    CELL cat;
    struct Cell_stats cell_stats;
    CELL *list;
    int row, nrows, ncols;
    int n;
    long count;

    fd = files->train_fd;
    cell = files->train_cell;

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* determine the categories in the map */
    I_init_signatures(S, files->nbands);
    Rast_init_cell_stats(&cell_stats);
    G_message(_("Finding training classes..."));
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	Rast_get_c_row(fd, cell, row);
	Rast_update_cell_stats(cell, ncols, &cell_stats);
    }
    G_percent(nrows, nrows, 2);

    /* convert this to an array */
    Rast_rewind_cell_stats(&cell_stats);
    n = 0;
    while (Rast_next_cell_stat(&cat, &count, &cell_stats)) {
	if (count > 1) {
	    I_new_signature(S);
	    S->sig[n].status = 1;
	    S->sig[n].npoints = count;
	    strncpy(S->sig[n].desc,
		      Rast_get_c_cat(&cat, &files->training_labels),
		      sizeof(S->sig[n].desc)
		);
	    n++;
	}
	else
	    G_warning(_("Training class %d only has one cell - this class will be ignored"),
		      cat);
    }

    if (n == 0)
	G_fatal_error(_("Training map has no classes"));

    list = (CELL *) G_calloc(n, sizeof(CELL));
    n = 0;
    Rast_rewind_cell_stats(&cell_stats);
    while (Rast_next_cell_stat(&cat, &count, &cell_stats))
	if (count > 1)
	    list[n++] = cat;

    Rast_free_cell_stats(&cell_stats);

    files->ncats = n;
    files->training_cats = list;

    G_message(n_("One class found", "%d classes found", files->ncats), files->ncats);

    return 0;
}
