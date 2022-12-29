#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "ncb.h"
#include "local_proto.h"

int readcell(int fd, int row, int nrows, int ncols)
{
    rotate_bufs();

    if (row < nrows)
	Rast_get_d_row(fd, ncb.buf[ncb.nsize - 1] + ncb.dist, row);
    else
	Rast_set_d_null_value(ncb.buf[ncb.nsize - 1] + ncb.dist, ncols);

    return 0;
}
