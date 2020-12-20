#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include "parms.h"


int parse(int argc, char *argv[], struct parms *parms)
{
    struct Option *group, *subgroup, *sigfile, *trainingmap;

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
    sigfile->gisprompt = "new,sig,sigfile";
    sigfile->description = _("Name for output file containing result signatures");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    parms->training_map = trainingmap->answer;
    parms->group = group->answer;
    parms->subgroup = subgroup->answer;
    parms->sigfile = sigfile->answer;

    /* check all the inputs */
    if (G_find_raster(parms->training_map, "") == NULL)
	G_fatal_error(_("Raster map <%s> not found"), parms->training_map);

    if (!I_find_group(parms->group))
	G_fatal_error(_("Group <%s> not found in current subproject"), parms->group);

    if (!I_find_subgroup(parms->group, parms->subgroup))
	G_fatal_error(_("Subgroup <%s> in group <%s> not found"), parms->subgroup, parms->group);
    
    /* GRASS parser fails to detect existing signature files as
     * detection needs answers from other parameters as group and subgroup.
     * Thus check is performed only now. */
    if (!G_get_overwrite() && I_find_signature_file(parms->group, parms->subgroup, "sig", parms->sigfile)) {
        G_fatal_error(_("option <%s>: <%s> exists. To overwrite, use the --overwrite flag"), 
                        sigfile->key, sigfile->answer);
    }
    
    return 0;
}
