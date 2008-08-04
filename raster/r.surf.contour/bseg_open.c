#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grass/gis.h>
#include "cseg.h"

int bseg_open(BSEG * bseg, int srows, int scols, int nsegs_in_memory)
{
    char *filename;
    int errflag;
    int fd;

    bseg->filename = NULL;
    bseg->fd = -1;
    bseg->name = NULL;
    bseg->mapset = NULL;

    filename = G_tempfile();
    if (-1 == (fd = creat(filename, 0666))) {
	G_warning("bseg_open(): unable to create segment file");
	return -2;
    }
    if (0 >
	(errflag =
	 segment_format(fd, G_window_rows(), (G_window_cols() + 7) / 8, srows,
			scols, sizeof(char)))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning("bseg_open(): could not write segment file");
	    return -1;
	}
	else {
	    G_warning("bseg_open(): illegal configuration parameter(s)");
	    return -3;
	}
    }
    close(fd);
    if (-1 == (fd = open(filename, 2))) {
	unlink(filename);
	G_warning("bseg_open(): unable to re-open segment file");
	return -4;
    }
    if (0 > (errflag = segment_init(&(bseg->seg), fd, nsegs_in_memory))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning("bseg_open(): could not read segment file");
	    return -5;
	}
	else {
	    G_warning("bseg_open(): out of memory");
	    return -6;
	}
    }
    bseg->filename = filename;
    bseg->fd = fd;
    return 0;
}
