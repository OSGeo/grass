#include <unistd.h>
#include <fcntl.h>
#include <grass/rowio.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "glob.h"
#include "filter.h"
#include "local_proto.h"

int perform_filter(const char *in_name, const char *out_name,
		   FILTER * filter, int nfilters, int repeat)
{
    int *in;
    int *out;
    int n;
    int pass;
    ROWIO *r;
    char *tmp1, *tmp2;
    int count;
    int row;
    int t;
    DCELL **cell;

    cell = G_malloc(nprocs * sizeof(DCELL*));
    for (t = 0; t < nprocs; t++) {
        cell[t] = Rast_allocate_d_buf();
    }

    in = G_malloc(nprocs * sizeof(int));
    out = G_malloc(nprocs * sizeof(int));
    r = G_malloc(nprocs * sizeof(ROWIO));

    count = 0;
    for (pass = 0; pass < repeat; pass++) {
	G_debug(1, "Pass %d", pass + 1);
	for (n = 0; n < nfilters; n++, count++) {
	    G_debug(1, "Filter %d", n + 1);

	    if (count == 0) {
            for (t = 0; t < nprocs; t++) {
                in[t] = Rast_open_old(in_name, "");

                G_debug(1, "Open raster map %s = %d", in_name, in[t]);
            }
            close(creat(tmp1 = G_tempfile(), 0666));

            for (t = 0; t < nprocs; t++) {
                out[t] = open(tmp1, 2);
                if (out[t] < 0) {
                    G_fatal_error(_("Unable to create temporary file"));
                }
            }
	    }
	    else if (count == 1) {

            G_debug(1, "Closing raster map");
            for (t = 0; t < nprocs; t++) {
                Rast_close(in[t]);
                in[t] = out[t];
            }
            close(creat(tmp2 = G_tempfile(), 0666));

            for (t = 0; t < nprocs; t++) {
                out[t] = open(tmp2, 2);
                if (out[t] < 0) {
                    G_fatal_error(_("Unable to create temporary file"));
                }
            }
	    }
	    else {
            int fd;

            G_debug(1, "Swap temp files");

            for (t = 0; t < nprocs; t++) {
                fd = in[t];
                in[t] = out[t];
                out[t] = fd;
            }
	    }

        for (t = 0; t < nprocs; t++) {
            Rowio_setup(&r[t], in[t], filter[n].size, buflen,
                count ? getrow : getmaprow, NULL);
        }

        execute_filter(r, out, &filter[n], cell);

        for (t = 0; t < nprocs; t++) {
            Rowio_release(&r[t]);
        }
	}
    }

    if (count == 1)
    for (t = 0; t < nprocs; t++)
        Rast_close(in[t]);
    else if (count > 1)
    for (t = 0; t < nprocs; t++)
        close(in[t]);

    /* copy final result to output raster map */
    in[MASTER] = out[MASTER];
    out[MASTER] = Rast_open_fp_new(out_name);

    G_message(_("Writing raster map <%s>"), out_name);
    for (row = 0; row < nrows; row++) {
        getrow(in[MASTER], cell[MASTER], row, buflen);
        Rast_put_d_row(out[MASTER], cell[MASTER]);
    }

    /* remove the temporary files before closing so that the Rast_close()
       has more disk to work with
     */
    if (count > 0)
	unlink(tmp1);
    if (count > 1)
	unlink(tmp2);
    Rast_close(out[MASTER]);

    return 0;
}
