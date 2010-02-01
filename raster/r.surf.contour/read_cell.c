#include "contour.h"
#include <grass/raster.h>
#include <grass/glocale.h>

CELL **read_cell(const char *name, const char *mapset)
{
    int nrows = Rast_window_rows();
    int ncols = Rast_window_cols();
    CELL *buf, **idx;
    int fd;
    int row;

    fd = Rast_open_old(name, mapset);

    buf = G_malloc((size_t) nrows * ncols * sizeof(CELL));
    idx = G_malloc(nrows * sizeof(CELL *));

    for (row = 0; row < nrows; row++) {
	idx[row] = &buf[row * ncols];

	Rast_get_c_row(fd, idx[row], row);
    }

    Rast_close(fd);

    return idx;
}

void free_cell(CELL **idx)
{
    G_free(idx[0]);
    G_free(idx);
}

