#include <grass/raster.h>
#include <grass/imagery.h>
#include "files.h"

int closefiles(struct files *files)
{
    int n;


    Rast_close(files->train_fd);
    for (n = 0; n < files->nbands; n++)
	Rast_close(files->band_fd[n]);

    return 0;
}
