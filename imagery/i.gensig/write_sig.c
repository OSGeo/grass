#include <stdlib.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "signature.h"
#include "parms.h"


int write_sigfile(struct parms *parms, struct Signature *S)
{
    FILE *fd;

    fd = I_fopen_signature_file_new(parms->group, parms->subgroup,
				    parms->sigfile);
    if (fd == NULL)
	G_fatal_error(_("Unable to create signature file [%s] for "
			"subgroup [%s] in group [%s]."),
		      parms->sigfile, parms->subgroup, parms->group);

    G_message(_("Writing signature file [%s] ..."), parms->sigfile);
    I_write_signatures(fd, S);
    G_message(_("Done."));

    return 0;
}
