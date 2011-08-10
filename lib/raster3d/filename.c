#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/

void
G3d_filename(char *path, const char *elementName, const char *mapName,
	     const char *mapset)
{
    G_file_name_misc(path, G3D_DIRECTORY, elementName, mapName, mapset);
}
