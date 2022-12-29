#include <unistd.h>
#include <fcntl.h>
#include <grass/glocale.h>
#include "seg.h"

int
seg_open(SSEG *sseg, GW_LARGE_INT nrows, GW_LARGE_INT ncols,
         int row_in_seg, int col_in_seg,
	 int nsegs_in_memory, int size_struct, int fill)
{
    char *filename;
    int errflag;

    sseg->filename = NULL;
    sseg->fd = -1;

    filename = G_tempfile();
    if (0 > (errflag = Segment_open(&(sseg->seg), filename, nrows, ncols,
                                    row_in_seg, col_in_seg,
				    size_struct, nsegs_in_memory))) {
	if (errflag == -1) {
	    G_warning(_("File name is invalid"));
	    return -1;
	}
	else if (errflag == -2) {
	    G_warning(_("File write error"));
	    return -2;
	}
	else if (errflag == -3) {
	    G_warning(_("Illegal parameters are passed"));
	    return -3;
	}
	else if (errflag == -4) {
	    G_warning(_("File could not be re-opened"));
	    return -4;
	}
	else if (errflag == -5) {
	    G_warning(_("Prepared file could not be read"));
	    return -5;
	}
	else if (errflag == -6) {
	    G_warning(_("Out of memory"));
	    return -6;
	}
    }

    sseg->filename = filename;

    return 0;
}

int seg_close(SSEG *sseg)
{
    Segment_close(&(sseg->seg));

    return 0;
}

int seg_put(SSEG *sseg, char *value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_put(&(sseg->seg), value, row, col) < 0) {
	G_warning(_("Unable to write segment file"));
	return -1;
    }
    return 0;
}

int seg_put_row(SSEG *sseg, char *value, GW_LARGE_INT row)
{
    if (Segment_put_row(&(sseg->seg), value, row) < 0) {
	G_warning(_("seg_put_row(): could not write segment file"));
	return -1;
    }
    return 0;
}

int seg_get(SSEG *sseg, char *value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_get(&(sseg->seg), value, row, col) < 0) {
	G_warning(_("Unable to read segment file"));
	return -1;
    }
    return 0;
}

int seg_get_row(SSEG *sseg, char *value, GW_LARGE_INT row)
{
    if (Segment_get_row(&(sseg->seg), value, row) < 0) {
	G_warning(_("Unable to read segment file"));
	return -1;
    }
    return 0;
}

int seg_flush(SSEG *sseg)
{
    Segment_flush(&(sseg->seg));
    return 0;
}
