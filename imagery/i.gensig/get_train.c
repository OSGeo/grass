#include <stdlib.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "signature.h"
#include "files.h"


int get_training_classes (struct files *files, struct Signature *S)
{
    int fd;
    CELL *cell;
    CELL cat;
    struct Cell_stats cell_stats;
    CELL *list;
    int row,nrows,ncols;
    int n;
    long count;

    fd = files->train_fd;
    cell = files->train_cell;

    nrows = G_window_rows();
    ncols = G_window_cols();

/* determine the categories in the map */
    I_init_signatures (S, files->nbands);
    G_init_cell_stats (&cell_stats);
    G_message(_("Finding training classes ..."));
    for (row = 0; row < nrows; row++)
    {
	G_percent (row, nrows, 2);
	if (G_get_c_raster_row(fd, cell, row) < 0)
            G_fatal_error(_("Unable to read raster map <%s> row %d"), cell, row);
	G_update_cell_stats (cell, ncols, &cell_stats);
    }
    G_percent (row, nrows, 2);

/* convert this to an array */
    G_rewind_cell_stats (&cell_stats);
    n = 0;
    while (G_next_cell_stat (&cat, &count, &cell_stats))
    {
	    if( count > 1)
	    {
		I_new_signature(S);
		S->sig[n].status = 1;
		S->sig[n].npoints = count;
		G_strncpy (S->sig[n].desc,
		           G_get_cat (cat, &files->training_labels),
	                   sizeof(S->sig[n].desc)
		          );
		n++;
	    }
	    else
		G_warning(_("Training class [%d] only has one cell - this class will be ignored."), cat);
    }

    if (n == 0)
	G_fatal_error(_("Training map has no classes."));

    list = (CELL *) G_calloc (n, sizeof(CELL));
    n = 0;
    G_rewind_cell_stats (&cell_stats);
    while (G_next_cell_stat (&cat, &count, &cell_stats))
	if (count > 1)
	    list[n++] = cat;

    G_free_cell_stats (&cell_stats);

    files->ncats = n;
    files->training_cats = list;

    if (files->ncats == 1)
        G_message(_("1 class."));
    else
        G_message(_("%d classes."), files->ncats);

    return 0;
}
