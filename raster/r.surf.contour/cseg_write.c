#include <grass/gis.h>
#include "cseg.h"

static char *me = "cseg_write_cell";

int cseg_write_cellfile(CSEG * cseg, char *map_name)
{
    int map_fd;
    int row, nrows;
    char msg[100];
    CELL *buffer;

    map_fd = G_open_cell_new(map_name);
    if (map_fd < 0) {
	sprintf(msg, "%s(): unable to open new map layer [%s]", me, map_name);
	G_warning(msg);
	return -1;
    }
    nrows = G_window_rows();
    buffer = G_allocate_cell_buf();
    for (row = 0; row < nrows; row++) {
	segment_get_row(&(cseg->seg), buffer, row);
	if (G_put_raster_row(map_fd, buffer, CELL_TYPE) < 0) {
	    G_free(buffer);
	    G_unopen_cell(map_fd);
	    sprintf(msg,
		    "%s(): unable to write new map layer [%s], row %d",
		    me, map_name, row);
	    G_warning(msg);
	    return -2;
	}
    }
    G_free(buffer);
    G_close_cell(map_fd);
    return 0;
}
