#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

void Rast3d_make_mapset_map_directory(const char *mapName)
{
    char buf[GNAME_MAX + sizeof(RASTER3D_DIRECTORY) + 2];

    sprintf(buf, "%s/%s", RASTER3D_DIRECTORY, mapName);
    G_make_mapset_element(buf);
}
