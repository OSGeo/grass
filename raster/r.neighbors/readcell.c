#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "ncb.h"
#include "local_proto.h"

int readcell(int fd, int row, int nrows, int ncols, int thread_id)
{
    rotate_bufs(thread_id);

    if (row >= 0 && row < nrows)
        Rast_get_d_row(fd, ncb.buf[thread_id][ncb.nsize - 1] + ncb.dist, row);
    else
        Rast_set_d_null_value(ncb.buf[thread_id][ncb.nsize - 1] + ncb.dist,
                              ncols);

    return 0;
}
