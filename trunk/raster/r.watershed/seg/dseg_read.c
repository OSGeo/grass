#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/segment.h>
#include "Gwater.h"

static char *me = "dseg_read_cell";

int dseg_read_cell(DSEG * dseg, char *map_name, char *mapset)
{
    GW_LARGE_INT row, rows;
    int map_fd;
    double *dbuffer;

    dseg->name = NULL;
    dseg->mapset = NULL;

    map_fd = Rast_open_old(map_name, mapset);
    rows = Rast_window_rows();
    dbuffer = Rast_allocate_d_buf();
    for (row = 0; row < rows; row++) {
	Rast_get_d_row(map_fd, dbuffer, row);
	if (Segment_put_row(&(dseg->seg), (DCELL *) dbuffer, row) < 0) {
	    G_free(dbuffer);
	    Rast_close(map_fd);
	    G_warning("%s(): unable to segment put row for [%s] in [%s]",
		      me, map_name, mapset);
	    return (-1);
	}
    }

    Rast_close(map_fd);
    G_free(dbuffer);

    dseg->name = G_store(map_name);
    dseg->mapset = G_store(mapset);

    return 0;
}
