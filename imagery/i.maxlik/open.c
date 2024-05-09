#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "global.h"
#include "local_proto.h"

int open_files(void)
{
    char *name, *mapset, **err;
    FILE *fd;
    int n;

    I_init_group_ref(&Ref);
    if (!I_find_group(group))
        G_fatal_error(_("Group <%s> not found in current mapset"), group);

    if (!I_find_subgroup(group, subgroup))
        G_fatal_error(_("Subgroup <%s> in group <%s> not found"), subgroup,
                      group);

    I_get_subgroup_ref(group, subgroup, &Ref);

    if (Ref.nfiles <= 1) {
        if (Ref.nfiles <= 0)
            G_fatal_error(
                _("Subgroup <%s> of group <%s> doesn't have any raster maps. "
                  "The subgroup must have at least 2 raster maps."),
                subgroup, group);
        else
            G_fatal_error(
                _("Subgroup <%s> of group <%s> only has 1 raster map. "
                  "The subgroup must have at least 2 raster maps."),
                subgroup, group);
    }

    fd = I_fopen_signature_file_old(sigfile);
    if (fd == NULL)
        G_fatal_error(_("Unable to open signature file <%s>"), sigfile);

    n = I_read_signatures(fd, &S);
    fclose(fd);
    if (n < 0)
        G_fatal_error(_("Unable to read signature file <%s>"), sigfile);

    if (S.nsigs > 255)
        G_fatal_error(_("<%s> has too many signatures (limit is 255)"),
                      sigfile);
<<<<<<< HEAD
<<<<<<< HEAD

    err = I_sort_signatures_by_semantic_label(&S, &Ref);
    if (err)
        G_fatal_error(_("Signature - group member semantic label mismatch.\n"
                        "Extra signatures for bands: %s\n"
                        "Imagery group bands without signatures: %s"),
                      err[0] ? err[0] : _("none"), err[1] ? err[1] : _("none"));
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

    err = I_sort_signatures_by_semantic_label(&S, &Ref);
    if (err)
        G_fatal_error(_("Signature - group member semantic label mismatch.\n"
                        "Extra signatures for bands: %s\n"
                        "Imagery group bands without signatures: %s"),
                      err[0] ? err[0] : _("none"), err[1] ? err[1] : _("none"));

    B = (double *)G_malloc(S.nsigs * sizeof(double));
    invert_signatures();

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    cell = (DCELL **)G_malloc(Ref.nfiles * sizeof(DCELL *));
    cellfd = (int *)G_malloc(Ref.nfiles * sizeof(int));
    P = (double *)G_malloc(Ref.nfiles * sizeof(double));
    for (n = 0; n < Ref.nfiles; n++) {
        cell[n] = Rast_allocate_d_buf();
        name = Ref.file[n].name;
        mapset = Ref.file[n].mapset;
        cellfd[n] = Rast_open_old(name, mapset);
<<<<<<< HEAD
=======
    cell = (DCELL **) G_malloc(Ref.nfiles * sizeof(DCELL *));
    cellfd = (int *)G_malloc(Ref.nfiles * sizeof(int));
    P = (double *)G_malloc(Ref.nfiles * sizeof(double));
    for (n = 0; n < Ref.nfiles; n++) {
	cell[n] = Rast_allocate_d_buf();
	name = Ref.file[n].name;
	mapset = Ref.file[n].mapset;
	cellfd[n] = Rast_open_old(name, mapset);
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
    cell = (DCELL **)G_malloc(Ref.nfiles * sizeof(DCELL *));
    cellfd = (int *)G_malloc(Ref.nfiles * sizeof(int));
    P = (double *)G_malloc(Ref.nfiles * sizeof(double));
    for (n = 0; n < Ref.nfiles; n++) {
        cell[n] = Rast_allocate_d_buf();
        name = Ref.file[n].name;
        mapset = Ref.file[n].mapset;
        cellfd[n] = Rast_open_old(name, mapset);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    }

    class_fd = Rast_open_c_new(class_name);
    class_cell = Rast_allocate_c_buf();

    reject_cell = NULL;
    if (reject_name) {
        reject_fd = Rast_open_c_new(reject_name);
        reject_cell = Rast_allocate_c_buf();
    }

    return 0;
}
