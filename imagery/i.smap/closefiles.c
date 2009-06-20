#include <unistd.h>
#include <grass/gis.h>
#include <grass/Rast.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include "bouman.h"
#include "local_proto.h"


int closefiles(struct parms *parms, struct files *files)
{
    int n;

    G_debug(1, "Creating support files for <%s>...", parms->output_map);

    for (n = 0; n < files->nbands; n++)
	Rast_close_cell(files->band_fd[n]);

    Rast_close_cell(files->output_fd);
    Rast_write_cats(parms->output_map, &files->output_labels);
    make_history(parms->output_map,
		 parms->group, parms->subgroup, parms->sigfile);

    return 0;
}
