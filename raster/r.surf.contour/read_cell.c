#include "contour.h"
#include <grass/raster.h>
#include <grass/glocale.h>

DCELL **read_cell(const char *name)
{
    int nrows = Rast_window_rows();
    int ncols = Rast_window_cols();
    DCELL *buf, **idx;
    int fd;
    int row;

    fd = Rast_open_old(name, "");
    
    buf = G_malloc((size_t) nrows * ncols * sizeof(DCELL));
    idx = G_malloc(nrows * sizeof(DCELL *));
    
    for (row = 0; row < nrows; row++) {
	idx[row] = &buf[row * ncols];

	Rast_get_d_row(fd, idx[row], row);
    }
    
    Rast_close(fd);
    
    return idx;
}

void free_cell(DCELL **idx)
{
    G_free(idx[0]);
    G_free(idx);
}
