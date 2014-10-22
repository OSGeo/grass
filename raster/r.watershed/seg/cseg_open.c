#include <grass/gis.h>
#include <unistd.h>
#include <fcntl.h>
#include "Gwater.h"


int cseg_open(CSEG * cseg, int srows, int scols, int nsegs_in_memory)
{
    char *filename;
    int errflag;
    int fd;

    cseg->filename = NULL;
    cseg->fd = -1;
    cseg->name = NULL;
    cseg->mapset = NULL;

    filename = G_tempfile();
    if (-1 == (fd = creat(filename, 0666))) {
	G_warning("cseg_open(): unable to create segment file");
	return -2;
    }
    if (0 >
	(errflag =
	 Segment_format(fd, Rast_window_rows(), Rast_window_cols(), srows, scols,
			sizeof(CELL)))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning("cseg_open(): could not write segment file");
	    return -1;
	}
	else {
	    G_warning("cseg_open(): illegal configuration parameter(s)");
	    return -3;
	}
    }
    close(fd);
    if (-1 == (fd = open(filename, 2))) {
	unlink(filename);
	G_warning("cseg_open(): unable to re-open segment file");
	return -4;
    }
    if (0 > (errflag = Segment_init(&(cseg->seg), fd, nsegs_in_memory))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning("cseg_open(): could not read segment file");
	    return -5;
	}
	else {
	    G_warning("cseg_open(): out of memory");
	    return -6;
	}
    }
    cseg->filename = filename;
    cseg->fd = fd;
    return 0;
}
