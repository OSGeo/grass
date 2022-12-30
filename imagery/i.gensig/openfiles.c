#include <stdlib.h>

#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

#include "parms.h"
#include "files.h"

<<<<<<< HEAD
<<<<<<< HEAD
int openfiles(struct parms *parms, struct files *files, struct Signature *S)
{
    struct Ref Ref; /* subgroup reference list */
    const char *mapset, *semantic_label;
=======

int openfiles(struct parms *parms, struct files *files, struct Signature *S)
{
    struct Ref Ref;		/* subgroup reference list */
    const char *mapset, *bandref;
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
int openfiles(struct parms *parms, struct files *files, struct Signature *S)
{
    struct Ref Ref; /* subgroup reference list */
    const char *mapset, *semantic_label;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
    int n;

    if (!I_get_subgroup_ref(parms->group, parms->subgroup, &Ref))
        G_fatal_error(
            _("Unable to read REF file for subgroup <%s> in group <%s>"),
            parms->subgroup, parms->group);

    if (Ref.nfiles <= 0)
        G_fatal_error(_("Subgroup <%s> in group <%s> contains no raster maps."),
                      parms->subgroup, parms->group);

    /* allocate file descriptors, and array of io buffers */
    files->nbands = Ref.nfiles;
    files->band_fd = (int *)G_calloc(Ref.nfiles, sizeof(int));
    files->band_cell = (DCELL **)G_calloc(Ref.nfiles, sizeof(DCELL *));

    /* open training map for reading */
    mapset = G_find_raster2(parms->training_map, "");
    files->train_fd = Rast_open_old(parms->training_map, mapset);
    files->train_cell = Rast_allocate_c_buf();

    /* prepare signature struct */
    I_init_signatures(S, Ref.nfiles);

    /* open all maps for reading and
<<<<<<< HEAD
<<<<<<< HEAD
       store semantic labels of imagery group bands */
    for (n = 0; n < Ref.nfiles; n++) {
        files->band_fd[n] = Rast_open_old(Ref.file[n].name, Ref.file[n].mapset);
        files->band_cell[n] = Rast_allocate_d_buf();
        semantic_label = Rast_get_semantic_label_or_name(Ref.file[n].name,
                                                         Ref.file[n].mapset);
        S->semantic_labels[n] = G_store(semantic_label);
=======
       store band references of imagery group bands */
    for (n = 0; n < Ref.nfiles; n++) {
	files->band_fd[n] =
	    Rast_open_old(Ref.file[n].name, Ref.file[n].mapset);
	files->band_cell[n] = Rast_allocate_d_buf();
        bandref = Rast_read_bandref(Ref.file[n].name, Ref.file[n].mapset);
        if (!bandref)
            G_fatal_error(_("Raster map <%s@%s> lacks band reference"),
                            Ref.file[n].name, Ref.file[n].mapset);
        S->bandrefs[n] = G_store(bandref);
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
       store semantic labels of imagery group bands */
    for (n = 0; n < Ref.nfiles; n++) {
        files->band_fd[n] = Rast_open_old(Ref.file[n].name, Ref.file[n].mapset);
        files->band_cell[n] = Rast_allocate_d_buf();
        semantic_label = Rast_get_semantic_label_or_name(Ref.file[n].name,
                                                         Ref.file[n].mapset);
        S->semantic_labels[n] = G_store(semantic_label);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
    }

    I_free_group_ref(&Ref);

    return 0;
}
