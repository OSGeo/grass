#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include "bouman.h"
#include "local_proto.h"


int closefiles(struct parms *parms, struct files *files)
{
    int n;

    G_debug(1, "Creating support files for <%s>...", parms->output_map);

    for (n = 0; n < files->nbands; n++)
	Rast_close(files->band_fd[n]);

    Rast_close(files->output_fd);
    Rast_write_cats(parms->output_map, &files->output_labels);
    make_history(parms->output_map,
		 parms->group, parms->subgroup, parms->sigfile);

    if (files->goodness_fd >= 0) {
	Rast_close(files->goodness_fd);
	make_history(parms->goodness_map,
		     parms->group, parms->subgroup, parms->sigfile);
    }

    return 0;
}
