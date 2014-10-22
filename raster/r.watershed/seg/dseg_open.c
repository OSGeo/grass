#include <grass/gis.h>
#include <unistd.h>
#include <fcntl.h>
#include <grass/segment.h>
#include "Gwater.h"


int dseg_open(DSEG * dseg, int srows, int scols, int nsegs_in_memory)
{
    char *filename;
    int errflag;
    int fd;

    dseg->filename = NULL;
    dseg->fd = -1;
    dseg->name = NULL;
    dseg->mapset = NULL;

    filename = G_tempfile();
    if (-1 == (fd = creat(filename, 0666))) {
	G_warning("dseg_open(): unable to create segment file");
	return -2;
    }
    if (0 >
	(errflag =
	 Segment_format(fd, Rast_window_rows(), Rast_window_cols(), srows, scols,
			sizeof(double)))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning("dseg_open(): could not write segment file");
	    return -1;
	}
	else {
	    G_warning("dseg_open(): illegal configuration parameter(s)");
	    return -3;
	}
    }
    close(fd);
    if (-1 == (fd = open(filename, 2))) {
	unlink(filename);
	G_warning("dseg_open(): unable to re-open segment file");
	return -4;
    }
    if (0 > (errflag = Segment_init(&(dseg->seg), fd, nsegs_in_memory))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning("dseg_open(): could not read segment file");
	    return -5;
	}
	else {
	    G_warning("dseg_open(): out of memory");
	    return -6;
	}
    }
    dseg->filename = filename;
    dseg->fd = fd;
    return 0;
}
