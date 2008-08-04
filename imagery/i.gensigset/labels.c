#include <grass/imagery.h>
#include "parms.h"
#include "files.h"

int read_training_labels(struct parms *parms, struct files *files)
{
    char *mapset;
    char *map;

    map = parms->training_map;
    mapset = G_find_cell(map, "");
    if (G_read_cats(map, mapset, &files->training_labels) < 0)
	G_init_cats((CELL) 0, "", &files->training_labels);

    return 0;
}
