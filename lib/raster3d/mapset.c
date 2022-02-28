#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

void Rast3d_make_mapset_map_directory(const char *mapName)
{
    G_make_mapset_dir_object(RASTER3D_DIRECTORY, mapName);
}
