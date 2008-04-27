#include <stdlib.h>
#include <grass/imagery.h>
#include "files.h"
#include "local_proto.h"

int 
read_training_map (CELL *class, int row, int ncols, struct files *files)
{
    if(G_get_c_raster_row (files->train_fd, files->train_cell, row) < 0) exit(1);
    lookup_class(files->train_cell, ncols, files->training_cats, files->ncats, class);

    return 0;
}

