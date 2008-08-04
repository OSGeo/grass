/* Function: cellfile
 **
 ** This function is a slightly modified version of the p.map function.
 ** Modified: Paul W. Carlson   April 1992
 */

#include <string.h>
#include "ps_info.h"
#include "local_proto.h"

int read_cell(char *name, char *mapset)
{
    char fullname[100];

    PS.do_colortable = 0;
    if (PS.cell_fd >= 0) {
	G_close_cell(PS.cell_fd);
	G_free(PS.cell_name);
	G_free(PS.cell_mapset);
	G_free_colors(&PS.colors);
	PS.cell_fd = -1;
    }

    sprintf(fullname, "%s in %s", name, mapset);

    if (G_read_colors(name, mapset, &PS.colors) == -1) {
	error(fullname, "", "can't read color table");
	return 0;
    }
    G_get_color_range(&PS.min_color, &PS.max_color, &PS.colors);

    /* open raster map for reading */
    if ((PS.cell_fd = G_open_cell_old(name, mapset)) < 0) {
	error(fullname, "", "can't open raster map");
	G_free_colors(&PS.colors);
	return 0;
    }

    strcpy(PS.celltitle, G_get_cell_title(name, mapset));
    G_strip(PS.celltitle);
    if (PS.celltitle[0] == 0)
	sprintf(PS.celltitle, "(%s)", name);
    PS.cell_name = G_store(name);
    PS.cell_mapset = G_store(mapset);
    PS.do_raster = 1;
    return 1;
}
