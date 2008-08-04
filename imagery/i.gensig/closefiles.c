#include <grass/imagery.h>
#include "files.h"

int closefiles(struct files *files)
{
    int n;


    G_close_cell(files->train_fd);
    for (n = 0; n < files->nbands; n++)
	G_close_cell(files->band_fd[n]);

    return 0;
}
