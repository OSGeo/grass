#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include "bouman.h"


int parse (int argc, char *argv[], struct parms *parms)
{
    struct Option *group, *subgroup, *sigfile, *output;
    struct Option *blocksize;
    struct Flag *quiet;
    struct Flag *ml;

    group = G_define_option();
    group->key = "group";
    group->description = _("imagery group");
    group->required = YES;
    group->type = TYPE_STRING;
    group->gisprompt = "old,group,group";

    subgroup = G_define_option();
    subgroup->key = "subgroup";
    subgroup->description = _("imagery subgroup");
    subgroup->required = YES;
    subgroup->type = TYPE_STRING;

    sigfile = G_define_option();
    sigfile->key = "signaturefile";
    sigfile->description = _("imagery signaturefile");
    sigfile->required = YES;
    sigfile->type = TYPE_STRING;

    blocksize = G_define_option();
    blocksize->key = "blocksize";
    blocksize->description = _("size of submatrix to process at one time");
    blocksize->required = NO;
    blocksize->type = TYPE_INTEGER;
    blocksize->answer = "128";

    output = G_define_option();
    output->key = "output";
    output->description = _("output raster map");
    output->required = YES;
    output->type = TYPE_STRING;
    output->gisprompt = "new,cell,raster";

    ml = G_define_flag();
    ml->key = 'm';
    ml->description = _("Use maximum likelihood estimation (instead of smap)");

    quiet = G_define_flag();
    quiet->key = 'q';
    quiet->description = _("Run quietly");

    if (G_parser(argc,argv)) exit(1);

    parms->quiet = quiet->answer;
    parms->ml = ml->answer;

    parms->output_map = output->answer;
    parms->group = group->answer;
    parms->subgroup = subgroup->answer;
    parms->sigfile = sigfile->answer;

    /* check all the inputs */
    if (!I_find_group(parms->group))
        G_fatal_error(_("Group [%s] not found."), parms->group);

    if (!I_find_subgroup(parms->group, parms->subgroup))
        G_fatal_error(_("Subgroup [%s] not found."), parms->subgroup);

    if (sscanf(blocksize->answer,"%d",&parms->blocksize) !=1
    || parms->blocksize <= 8) parms->blocksize = 8;

    return 0;
}
