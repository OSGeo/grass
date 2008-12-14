#include <stdlib.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "files.h"
#include "local_proto.h"

int read_training_map(CELL * class, int row, int ncols, struct files *files)
{
    if (G_get_c_raster_row(files->train_fd, files->train_cell, row) < 0)
	G_fatal_error(_("Unable to read raster map row %d"),
		      row);
    lookup_class(files->train_cell, ncols, files->training_cats, files->ncats,
		 class);

    return 0;
}
