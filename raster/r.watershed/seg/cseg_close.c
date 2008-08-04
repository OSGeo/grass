#include <grass/gis.h>
#include <unistd.h>
#include <grass/segment.h>
#include "cseg.h"

int cseg_close(CSEG * cseg)
{
    segment_release(&(cseg->seg));
    close(cseg->fd);
    unlink(cseg->filename);
    if (cseg->name) {
	G_free(cseg->name);
	cseg->name = NULL;
    }
    if (cseg->mapset) {
	G_free(cseg->mapset);
	cseg->mapset = NULL;
    }
    return 0;
}
