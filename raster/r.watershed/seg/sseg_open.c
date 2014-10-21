#include <grass/gis.h>
#include <unistd.h>
#include <fcntl.h>
#include <grass/segment.h>
#include "Gwater.h"

int
seg_open(SSEG * sseg, GW_LARGE_INT nrows, GW_LARGE_INT ncols, int row_in_seg, int col_in_seg,
	 int nsegs_in_memory, int size_struct)
{
    char *filename;
    int errflag;
    int fd;

    sseg->filename = NULL;
    sseg->fd = -1;

    filename = G_tempfile();
    if (-1 == (fd = creat(filename, 0666))) {
	G_warning("seg_open(): unable to create segment file");
	return -2;
    }
    if (0 > (errflag = Segment_format(fd, nrows, ncols,
				      row_in_seg, col_in_seg, size_struct))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning("seg_open(): could not write segment file");
	    return -1;
	}
	else {
	    G_warning("seg_open(): illegal configuration parameter(s)");
	    return -3;
	}
    }
    close(fd);
    if (-1 == (fd = open(filename, 2))) {
	unlink(filename);
	G_warning("seg_open(): unable to re-open segment file");
	return -4;
    }
    if (0 > (errflag = Segment_init(&(sseg->seg), fd, nsegs_in_memory))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning("seg_open(): could not read segment file");
	    return -5;
	}
	else {
	    G_warning("seg_open(): out of memory");
	    return -6;
	}
    }
    sseg->filename = filename;
    sseg->fd = fd;
    return 0;
}
