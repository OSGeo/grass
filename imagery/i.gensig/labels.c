#include <grass/imagery.h>
#include "parms.h"
#include "files.h"

void read_training_labels(struct parms *parms, struct files *files)
{
    const char *mapset;
    const char *map;

    map = parms->training_map;
    mapset = G_find_cell2(map, "");
    if (G_read_cats(map, mapset, &files->training_labels) < 0)
	G_init_cats((CELL) 0, "", &files->training_labels);
}
