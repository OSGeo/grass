#include <stdlib.h>
#include <grass/imagery.h>
#include "files.h"
#include "parms.h"

int get_training_classes (struct parms *parms,
    struct files *files, struct SigSet *S)
{
    int fd;
    CELL *cell;
    CELL cat;
    struct Cell_stats cell_stats;
    CELL *list;
    int row,nrows,ncols;
    int i,n;
    long count;
    struct ClassSig *Sig;

    fd = files->train_fd;
    cell = files->train_cell;

    nrows = G_window_rows();
    ncols = G_window_cols();

/* determine the non-zero categories in the map */
    I_InitSigSet (S);
    I_SigSetNBands (S, files->nbands);
    I_SetSigTitle(S, G_get_cats_title (&files->training_labels));

    G_init_cell_stats (&cell_stats);
    fprintf (stderr, "Finding training classes ...");
    for (row = 0; row < nrows; row++)
    {
	G_percent (row, nrows, 2);
	if (G_get_c_raster_row(fd, cell, row) < 0) exit(1);
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
		Sig = I_NewClassSig(S);
		I_SetClassTitle (Sig, G_get_cat (cat, &files->training_labels));
		Sig->classnum = cat;
	    /* initialize this class with maxsubclasses (by allocating them) */
		for (i = 0; i < parms->maxsubclasses; i++)
		    I_NewSubSig(S, Sig);
		I_AllocClassData (S, Sig, count);
		n++;
	    }
	    else
		fprintf (stderr,
		    "WARNING: Training class [%d] only has one cell - this class will be ignored\n", cat);
    }

    if (n==0)
    {
	fprintf (stderr, "ERROR: training map has no classes\n");
	exit(1);
    }

    list = (CELL *) G_calloc (n, sizeof(CELL));
    n = 0;
    G_rewind_cell_stats (&cell_stats);
    while (G_next_cell_stat (&cat, &count, &cell_stats))
	if (count > 1)
	    list[n++] = cat;

    G_free_cell_stats (&cell_stats);

    files->ncats = n;
    files->training_cats = list;
fprintf (stderr, "%d class%s\n", files->ncats, files->ncats==1?"":"es" );

    return 0;
}
