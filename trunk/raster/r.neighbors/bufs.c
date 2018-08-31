#include <grass/gis.h>
#include <grass/raster.h>
#include "ncb.h"

/*
   allocate the i/o bufs

   the i/o bufs will be rotated by the read operation so that the
   last row read will be in the last i/o buf

 */

int allocate_bufs(void)
{
    int i;
    int bufsize;

    bufsize = (Rast_window_cols() + 2 * ncb.dist) * sizeof(DCELL);

    ncb.buf = (DCELL **) G_malloc(ncb.nsize * sizeof(DCELL *));
    for (i = 0; i < ncb.nsize; i++) {
	ncb.buf[i] = (DCELL *) G_malloc(bufsize);
	Rast_set_d_null_value(ncb.buf[i], Rast_window_cols() + 2 * ncb.dist);
    }

    return 0;
}

int rotate_bufs(void)
{
    DCELL *temp;
    int i;

    temp = ncb.buf[0];

    for (i = 1; i < ncb.nsize; i++)
	ncb.buf[i - 1] = ncb.buf[i];

    ncb.buf[ncb.nsize - 1] = temp;

    return 0;
}
