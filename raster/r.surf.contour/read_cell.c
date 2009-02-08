#include "contour.h"
#include <grass/glocale.h>

CELL **read_cell(const char *name, const char *mapset)
{
    int nrows = G_window_rows();
    int ncols = G_window_cols();
    CELL *buf, **idx;
    int fd;
    int row;

    fd = G_open_cell_old(name, mapset);
    if (fd < 0)
	G_fatal_error(_("unable to open map <%s> in <%s>"),
		      name, mapset);

    buf = G_malloc((size_t) nrows * ncols * sizeof(CELL));
    idx = G_malloc(nrows * sizeof(CELL *));

    for (row = 0; row < nrows; row++) {
	idx[row] = &buf[row * ncols];

	if (G_get_map_row(fd, idx[row], row) < 0)
	    G_fatal_error(_("unable to read map <%s> in <%s>"),
			  name, mapset);
    }

    G_close_cell(fd);

    return idx;
}

void free_cell(CELL **idx)
{
    G_free(idx[0]);
    G_free(idx);
}

