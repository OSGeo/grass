#include <stdlib.h>
#include "globals.h"
#include "local_proto.h"

/*
 * run etc/i.find command find all cell, vect files
 * in the target location.
 */
int find_target_files(void)
{
    char command[1024];

    select_target_env();
    sprintf(command, "%s/etc/i.find %s %s cell %s dig %s",
	    G_gisbase(), G_location(), G_mapset(), cell_list, vect_list);
    select_current_env();

    system(command);

    return 0;
}
