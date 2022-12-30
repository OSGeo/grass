#include <stdlib.h>

#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

#include "bouman.h"
#include "local_proto.h"

<<<<<<< HEAD
<<<<<<< HEAD
int openfiles(struct parms *parms, struct files *files, struct SigSet *S)
{
    FILE *fd;
    struct Ref Ref; /* subgroup reference list */
    int n;
    char **err;
=======

=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
int openfiles(struct parms *parms, struct files *files, struct SigSet *S)
{
    FILE *fd;
    struct Ref Ref; /* subgroup reference list */
    int n;
    char **err;

<<<<<<< HEAD
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))

=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
    if (!I_get_subgroup_ref(parms->group, parms->subgroup, &Ref))
        G_fatal_error(
            _("Unable to read REF file for subgroup <%s> in group <%s>"),
            parms->subgroup, parms->group);

    if (Ref.nfiles <= 0)
        G_fatal_error(_("Subgroup <%s> in group <%s> contains no raster maps"),
                      parms->subgroup, parms->group);
<<<<<<< HEAD

    fd = I_fopen_sigset_file_old(parms->sigfile);
    if (fd == NULL)
        G_fatal_error(_("Unable to read signature file <%s>"), parms->sigfile);

    if (I_ReadSigSet(fd, S) < 0 || Ref.nfiles != S->nbands)
        G_fatal_error(_("Signature file <%s> is invalid"), parms->sigfile);

    if (S->ClassSig == NULL || S->title == NULL)
        G_fatal_error(_("Signature file <%s> is empty"), parms->sigfile);

    fclose(fd);

    err = I_SortSigSetBySemanticLabel(S, &Ref);
    if (err)
        G_fatal_error(_("Signature - group member semantic label mismatch.\n"
                        "Extra signatures for bands: %s\n"
                        "Imagery group bands without signatures: %s"),
                      err[0] ? err[0] : _("none"), err[1] ? err[1] : _("none"));
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

    fd = I_fopen_sigset_file_old(parms->sigfile);
    if (fd == NULL)
        G_fatal_error(_("Unable to read signature file <%s>"), parms->sigfile);

    if (I_ReadSigSet(fd, S) < 0 || Ref.nfiles != S->nbands)
        G_fatal_error(_("Signature file <%s> is invalid"), parms->sigfile);

    if (S->ClassSig == NULL || S->title == NULL)
        G_fatal_error(_("Signature file <%s> is empty"), parms->sigfile);

    fclose(fd);

    err = I_SortSigSetBySemanticLabel(S, &Ref);
    if (err)
        G_fatal_error(_("Signature - group member semantic label mismatch.\n"
                        "Extra signatures for bands: %s\n"
                        "Imagery group bands without signatures: %s"),
                      err[0] ? err[0] : _("none"), err[1] ? err[1] : _("none"));

    /* allocate file descriptors, and io buffer */
    files->cellbuf = Rast_allocate_d_buf();
    files->outbuf = Rast_allocate_c_buf();

    files->isdata = G_malloc(Rast_window_cols());

    files->nbands = Ref.nfiles;
    files->band_fd = (int *)G_calloc(Ref.nfiles, sizeof(int));

    /* open all group maps for reading */
    for (n = 0; n < Ref.nfiles; n++)
        files->band_fd[n] = open_cell_old(Ref.file[n].name, Ref.file[n].mapset);

    /* open output map */
    files->output_fd = open_cell_new(parms->output_map);

    if (parms->goodness_map)
        files->goodness_fd = Rast_open_new(parms->goodness_map, FCELL_TYPE);
    else
        files->goodness_fd = -1;

    return 0;
}
