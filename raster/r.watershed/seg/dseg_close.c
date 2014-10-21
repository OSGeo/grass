#include <grass/gis.h>
#include <unistd.h>
#include "Gwater.h"

int dseg_close(DSEG * dseg)
{
    Segment_release(&(dseg->seg));
    close(dseg->fd);
    unlink(dseg->filename);
    if (dseg->name) {
	G_free(dseg->name);
	dseg->name = NULL;
    }
    if (dseg->mapset) {
	G_free(dseg->mapset);
	dseg->mapset = NULL;
    }
    return 0;
}
