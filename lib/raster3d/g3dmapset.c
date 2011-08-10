#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/

void G3d_makeMapsetMapDirectory(const char *mapName)
{
    char buf[GNAME_MAX + sizeof(G3D_DIRECTORY) + 2];

    sprintf(buf, "%s/%s", G3D_DIRECTORY, mapName);
    G__make_mapset_element(buf);
}
