#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/

void
G3d_makeMapsetMapDirectory  (char *mapName)

{
  char buf[200];

  sprintf(buf, "%s/%s", G3D_DIRECTORY, mapName);
  G__make_mapset_element (buf);
}

