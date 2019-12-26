#include <grass/gis.h>
#include <unistd.h>
#include <fcntl.h>
#include <grass/segment.h>
#include <grass/glocale.h>
#include "Gwater.h"

int
seg_open(SSEG * sseg, GW_LARGE_INT rows, GW_LARGE_INT cols, int row_in_seg,
	 int col_in_seg, int nsegs_in_memory, int size_struct)
{
    char *filename;
    int errflag;

    sseg->filename = NULL;
    sseg->fd = -1;

    filename = G_tempfile();
    if (0 > (errflag = Segment_open(&(sseg->seg), filename, rows, cols,
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
