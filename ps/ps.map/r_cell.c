/* Function: cellfile
 **
 ** This function is a slightly modified version of the p.map function.
 ** Modified: Paul W. Carlson   April 1992
 */

#include <string.h>
#include <grass/raster.h>
#include "local_proto.h"

int read_cell(char *name, char *subproject)
{
    /* full name can be "name@subproject in subproject" */
    char fullname[GNAME_MAX + 2 * GMAPSET_MAX + 4];

    PS.do_colortable = 0;
    if (PS.cell_fd >= 0) {
	Rast_close(PS.cell_fd);
	G_free(PS.cell_name);
	Rast_free_colors(&PS.colors);
	PS.cell_fd = -1;
    }

    sprintf(fullname, "%s in %s", name, subproject);

    if (Rast_read_colors(name, subproject, &PS.colors) == -1) {
	error(fullname, "", "can't read color table");
	return 0;
    }
    Rast_get_c_color_range(&PS.min_color, &PS.max_color, &PS.colors);

    /* open raster map for reading */
    PS.cell_fd = Rast_open_old(name, subproject);

    strcpy(PS.celltitle, Rast_get_cell_title(name, subproject));
    G_strip(PS.celltitle);
    if (PS.celltitle[0] == 0)
	sprintf(PS.celltitle, "(%s)", name);
    PS.cell_name = G_store(name);
    PS.cell_subproject = G_store(subproject);
    PS.do_raster = 1;
    return 1;
}
