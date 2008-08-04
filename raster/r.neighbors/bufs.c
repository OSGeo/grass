#include <grass/gis.h>
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

    bufsize = (G_window_cols() + 2 * ncb.nsize) * sizeof(DCELL);

    ncb.buf = (DCELL **) G_malloc(ncb.nsize * sizeof(DCELL *));
    for (i = 0; i < ncb.nsize; i++) {
	ncb.buf[i] = (DCELL *) G_malloc(bufsize);
	G_set_d_null_value(ncb.buf[i], G_window_cols() + 2 * ncb.nsize);
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
