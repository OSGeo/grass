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
    int in;
    int out;
    int n;
    int pass;
    ROWIO r;
    char *tmp1, *tmp2;
    int count;
    int row;
    DCELL *cell;


    cell = Rast_allocate_d_buf();

    count = 0;
    for (pass = 0; pass < repeat; pass++) {
	G_debug(1, "Pass %d", pass + 1);
	for (n = 0; n < nfilters; n++, count++) {
	    G_debug(1, "Filter %d", n + 1);

	    if (count == 0) {
		in = Rast_open_old(in_name, "");

		G_debug(1, "Open raster map %s = %d", in_name, in);

		close(creat(tmp1 = G_tempfile(), 0666));
		out = open(tmp1, 2);
		if (out < 0)
		    G_fatal_error(_("Unable to create temporary file"));
	    }
	    else if (count == 1) {

		G_debug(1, "Closing raster map");

		Rast_close(in);
		in = out;
		close(creat(tmp2 = G_tempfile(), 0666));
		out = open(tmp2, 2);
		if (out < 0)
		    G_fatal_error(_("Unable to create temporary file"));
	    }
	    else {
		int fd;

		G_debug(1, "Swap temp files");

		fd = in;
		in = out;
		out = fd;
	    }

	    Rowio_setup(&r, in, filter[n].size, buflen,
			count ? getrow : getmaprow, NULL);

	    execute_filter(&r, out, &filter[n], cell);

	    Rowio_release(&r);
	}
    }

    if (count == 1)
	Rast_close(in);
    else if (count > 1)
	close(in);

    /* copy final result to output raster map */
    in = out;
    out = Rast_open_fp_new(out_name);

    G_message(_("Writing raster map <%s>"), out_name);
    for (row = 0; row < nrows; row++) {
	getrow(in, cell, row, buflen);
	Rast_put_d_row(out, cell);
    }

    /* remove the temporary files before closing so that the Rast_close()
       has more disk to work with
     */
    if (count > 0)
	unlink(tmp1);
    if (count > 1)
	unlink(tmp2);
    Rast_close(out);

    return 0;
}
