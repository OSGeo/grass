#include <stdlib.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "parms.h"
#include "files.h"


int openfiles(struct parms *parms, struct files *files)
{
    struct Ref Ref;		/* subgroup reference list */
    char *mapset;
    int n;


    if (!I_get_subgroup_ref(parms->group, parms->subgroup, &Ref))
	G_fatal_error(_("Unable to read REF file for subgroup [%s] in group [%s]."),
		      parms->subgroup, parms->group);

    if (Ref.nfiles <= 0)
	G_fatal_error(_("Subgroup [%s] in group [%s] contains no files."),
		      parms->subgroup, parms->group);

    /* allocate file descriptors, and array of io buffers */
    files->nbands = Ref.nfiles;
    files->band_fd = (int *)G_calloc(Ref.nfiles, sizeof(int));
    files->band_cell = (DCELL **) G_calloc(Ref.nfiles, sizeof(DCELL *));

    /* open training map for reading */
    mapset = G_find_cell2(parms->training_map, "");
    files->train_fd = G_open_cell_old(parms->training_map, mapset);
    if (files->train_fd < 0)
	G_fatal_error(_("Unable to open training map <%s>"),
		      parms->training_map);
    files->train_cell = G_allocate_c_raster_buf();

    /* open all maps for reading */
    for (n = 0; n < Ref.nfiles; n++) {
	files->band_fd[n] =
	    G_open_cell_old(Ref.file[n].name, Ref.file[n].mapset);
	if (files->band_fd[n] < 0)
	    G_fatal_error(_("Unable to open training map <%s in %s>"),
			  Ref.file[n].name, Ref.file[n].mapset);
	files->band_cell[n] = G_allocate_d_raster_buf();
    }

    return 0;
}
