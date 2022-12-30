#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include "parms.h"

int parse(int argc, char *argv[], struct parms *parms)
{
    struct Option *group, *subgroup, *sigfile, *trainingmap;
    char xmapset[GMAPSET_MAX];

    trainingmap = G_define_standard_option(G_OPT_R_MAP);
    trainingmap->key = "trainingmap";
    trainingmap->description = _("Ground truth training map");

    group = G_define_standard_option(G_OPT_I_GROUP);

    subgroup = G_define_standard_option(G_OPT_I_SUBGROUP);

    sigfile = G_define_option();
    sigfile->key = "signaturefile";
    sigfile->type = TYPE_STRING;
    sigfile->key_desc = "name";
    sigfile->required = YES;
    sigfile->gisprompt = "new,signatures/sig,sigfile";
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    sigfile->description =
        _("Name for output file containing result signatures");
=======
    sigfile->description = _("Name for output file containing result signatures");
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
    sigfile->description =
        _("Name for output file containing result signatures");
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    sigfile->description =
        _("Name for output file containing result signatures");
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    parms->training_map = trainingmap->answer;
    parms->group = group->answer;
    parms->subgroup = subgroup->answer;

    /* check all the inputs */
    if (G_find_raster(parms->training_map, "") == NULL)
        G_fatal_error(_("Raster map <%s> not found"), parms->training_map);

    if (!I_find_group(parms->group))
        G_fatal_error(_("Group <%s> not found in current mapset"),
                      parms->group);

    if (!I_find_subgroup(parms->group, parms->subgroup))
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        G_fatal_error(_("Subgroup <%s> in group <%s> not found"),
                      parms->subgroup, parms->group);

    if (G_unqualified_name(sigfile->answer, G_mapset(), parms->sigfile,
                           xmapset) < 0)
<<<<<<< HEAD
=======
	G_fatal_error(_("Subgroup <%s> in group <%s> not found"), parms->subgroup, parms->group);

    if (G_unqualified_name(sigfile->answer, G_mapset(), parms->sigfile, xmapset) < 0)
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        G_fatal_error(_("Subgroup <%s> in group <%s> not found"),
                      parms->subgroup, parms->group);

    if (G_unqualified_name(sigfile->answer, G_mapset(), parms->sigfile,
                           xmapset) < 0)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        G_fatal_error(_("<%s> does not match the current mapset"), xmapset);

    if (G_legal_filename(parms->sigfile) < 0)
        G_fatal_error(_("<%s> is an illegal file name"), parms->sigfile);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

=======
    
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    return 0;
}
