#include <grass/raster.h>
#include <grass/imagery.h>
#include "parms.h"
#include "files.h"

void read_training_labels(struct parms *parms, struct files *files)
{
    const char *subproject;
    const char *map;

    map = parms->training_map;
    subproject = G_find_raster2(map, "");
    if (Rast_read_cats(map, subproject, &files->training_labels) < 0)
	Rast_init_cats("", &files->training_labels);
}
