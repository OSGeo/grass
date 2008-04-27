#include <stdlib.h>
#include <grass/imagery.h>
#include "files.h"
#include "parms.h"

int openfiles (struct parms *parms, struct files *files)
{
    struct Ref Ref;	/* subgroup reference list */
    CELL *open_cell();  /* opens existing rastermaps and returns fd and allocated io buffer */
    int n;


    if (!I_get_subgroup_ref (parms->group, parms->subgroup, &Ref))
    {
	fprintf (stderr,
	     "ERROR: unable to read REF file for subgroup [%s] in group [%s]\n",
		parms->subgroup, parms->group);
	exit(1);
    }
    if (Ref.nfiles <= 0)
    {
	fprintf (stderr, "ERROR: subgroup [%s] in group [%s] contains no files\n",
		parms->subgroup, parms->group);
	exit(1);
    }

    /* allocate file descriptors, and array of io buffers */
    files->nbands    = Ref.nfiles;
    files->band_fd   = (int *) G_calloc (Ref.nfiles, sizeof(int));
    files->band_cell = (CELL **) G_calloc (Ref.nfiles, sizeof(CELL *));

    /* open all maps for reading */
    files->train_cell = open_cell (parms->training_map, NULL, &files->train_fd);
    for (n = 0; n < Ref.nfiles; n++)
	files->band_cell[n] = open_cell (Ref.file[n].name, Ref.file[n].mapset, &files->band_fd[n]);

    return 0;
}
