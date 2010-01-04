#include <stdlib.h>

#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

#include "files.h"
#include "local_proto.h"

int read_training_map(CELL * class, int row, int ncols, struct files *files)
{
    Rast_get_c_row(files->train_fd, files->train_cell, row);
    lookup_class(files->train_cell, ncols, files->training_cats, files->ncats,
		 class);

    return 0;
}
