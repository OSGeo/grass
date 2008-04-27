#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include "parms.h"


int 
parse (int argc, char *argv[], struct parms *parms)
{
    struct Option *group, *subgroup, *sigfile, *trainingmap;

    trainingmap = G_define_option();
    trainingmap->key = "trainingmap";
    trainingmap->description = _("Ground truth training map");
    trainingmap->required = YES;
    trainingmap->type = TYPE_STRING;
    trainingmap->gisprompt = "old,cell,raster";

    group = G_define_option();
    group->key = "group";
    group->description = _("Imagery group");
    group->required = YES;
    group->type = TYPE_STRING;
    group->gisprompt = "old,group,group";

    subgroup = G_define_option();
    subgroup->key = "subgroup";
    subgroup->description = _("Subgroup containing image files");
    subgroup->required = YES;
    subgroup->type = TYPE_STRING;

    sigfile = G_define_option();
    sigfile->key = "signaturefile";
    sigfile->description = _("Resultant signature file");
    sigfile->required = YES;
    sigfile->type = TYPE_STRING;

    if (G_parser(argc,argv)) exit(1);

    parms->training_map = trainingmap->answer;
    parms->group = group->answer;
    parms->subgroup = subgroup->answer;
    parms->sigfile = sigfile->answer;

    /* check all the inputs */
    if(G_find_cell(parms->training_map, "") == NULL)
        G_fatal_error(_("Raster map <%s> not found"), parms->training_map);

    if (!I_find_group(parms->group))
        G_fatal_error(_("Group <%s> not found"), parms->group);

    if (!I_find_subgroup(parms->group, parms->subgroup))
        G_fatal_error(_("Subgroup <%s> not found"), parms->subgroup);

    return 0;
}
