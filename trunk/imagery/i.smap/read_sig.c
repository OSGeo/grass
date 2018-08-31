#include <stdlib.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "bouman.h"


int read_signatures(struct parms *parms, struct SigSet *S)
{
    FILE *fd;
    struct Ref Ref;

    if (!I_get_subgroup_ref(parms->group, parms->subgroup, &Ref))
	G_fatal_error(_("Unable to read REF file for subgroup <%s> in group <%s>"),
		      parms->subgroup, parms->group);

    if (Ref.nfiles <= 0)
	G_fatal_error(_("Subgroup <%s> in group <%s> contains no raster maps"),
		      parms->subgroup, parms->group);

    fd = I_fopen_sigset_file_old(parms->group, parms->subgroup,
				 parms->sigfile);
    if (fd == NULL)
	G_fatal_error(_("Unable to read signature file <%s>"),
		      parms->sigfile);
    
    if (I_ReadSigSet(fd, S) < 0 || Ref.nfiles != S->nbands)
	G_fatal_error(_("Signature file <%s> is invalid"), parms->sigfile);

    if (S->ClassSig == NULL || S->title == NULL)
	G_fatal_error(_("Signature file <%s> is empty"), parms->sigfile);

    fclose(fd);

    return 0;
}
