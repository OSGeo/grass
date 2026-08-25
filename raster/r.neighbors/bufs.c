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
    int i, t;
    int bufsize;

    bufsize = (Rast_window_cols() + 2 * ncb.dist) * sizeof(DCELL);

    ncb.buf = G_malloc(ncb.threads * sizeof(DCELL **));
    for (t = 0; t < ncb.threads; t++) {
        ncb.buf[t] = (DCELL **)G_malloc(ncb.nsize * sizeof(DCELL *));
        for (i = 0; i < ncb.nsize; i++) {
            ncb.buf[t][i] = (DCELL *)G_malloc(bufsize);
            Rast_set_d_null_value(ncb.buf[t][i],
                                  Rast_window_cols() + 2 * ncb.dist);
        }
    }

    return 0;
}

int rotate_bufs(int thread_id)
{
    DCELL *temp;
    int i;

    temp = ncb.buf[thread_id][0];

    for (i = 1; i < ncb.nsize; i++)
        ncb.buf[thread_id][i - 1] = ncb.buf[thread_id][i];

    ncb.buf[thread_id][ncb.nsize - 1] = temp;

    return 0;
}
