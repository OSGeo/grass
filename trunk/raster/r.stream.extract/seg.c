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
    int fd;

    sseg->filename = NULL;
    sseg->fd = -1;

    filename = G_tempfile();
    if (-1 == (fd = creat(filename, 0666))) {
	G_warning(_("Unable to create segment file"));
	return -2;
    }
    if (fill)
	errflag = Segment_format(fd, nrows, ncols, row_in_seg,
	                         col_in_seg, size_struct);
    else
	errflag = Segment_format_nofill(fd, nrows, ncols, row_in_seg,
	                         col_in_seg, size_struct);

    if (0 > errflag) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning(_("Unable to write segment file"));
	    return -1;
	}
	else {
	    G_warning(_("Illegal configuration parameter(s)"));
	    return -3;
	}
    }
    close(fd);
    if (-1 == (fd = open(filename, 2))) {
	unlink(filename);
	G_warning(_("Unable to re-open file '%s'"), filename);
	return -4;
    }
    if (0 > (errflag = Segment_init(&(sseg->seg), fd, nsegs_in_memory))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning(_("Unable to read segment file"));
	    return -5;
	}
	else {
	    G_warning(_("Out of memory"));
	    return -6;
	}
    }
    sseg->filename = filename;
    sseg->fd = fd;
    return 0;
}

int seg_close(SSEG *sseg)
{
    Segment_release(&(sseg->seg));
    close(sseg->fd);
    unlink(sseg->filename);
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
