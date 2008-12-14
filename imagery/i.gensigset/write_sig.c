#include <stdlib.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "parms.h"

int write_sigfile(struct parms *parms, struct SigSet *S)
{
    FILE *fd;

    fd = I_fopen_sigset_file_new(parms->group, parms->subgroup,
				 parms->sigfile);
    if (fd == NULL) {
	G_fatal_error(_("Unable to create signature file <%s>"),
		      parms->sigfile);
    } 
    G_verbose_message(_("Writing signatures..."));
    I_WriteSigSet(fd, S);
    fclose(fd);

    return 0;
}
