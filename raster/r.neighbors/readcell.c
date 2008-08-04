#include <unistd.h>
#include <grass/gis.h>
#include "ncb.h"
#include "local_proto.h"

int readcell(int fd, int row, int nrows, int ncols)
{
    rotate_bufs();

    if (row < nrows)
	G_get_d_raster_row(fd, ncb.buf[ncb.nsize - 1] + ncb.dist, row);
    else
	G_set_d_null_value(ncb.buf[ncb.nsize - 1] + ncb.dist, ncols);

    return 0;
}
