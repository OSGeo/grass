#include <grass/gis.h>
#include <unistd.h>
#include <fcntl.h>
#include <grass/segment.h>
#include <grass/glocale.h>
#include "Gwater.h"


int dseg_open(DSEG * dseg, int srows, int scols, int nsegs_in_memory)
{
    char *filename;
    int errflag;

    dseg->filename = NULL;
    dseg->fd = -1;
    dseg->name = NULL;
    dseg->mapset = NULL;

    filename = G_tempfile();
    if (0 > (errflag = Segment_open(&(dseg->seg), filename, Rast_window_rows(),
				    Rast_window_cols(), srows, scols,
				    sizeof(DCELL), nsegs_in_memory))) {
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

    dseg->filename = filename;

    return 0;
}
