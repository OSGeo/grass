#include <unistd.h>
#include <grass/gis.h>
#include "cseg.h"

int bseg_close(BSEG * bseg)
{
    segment_release(&(bseg->seg));
    close(bseg->fd);
    unlink(bseg->filename);
    if (bseg->name) {
	G_free(bseg->name);
	bseg->name = NULL;
    }
    if (bseg->mapset) {
	G_free(bseg->mapset);
	bseg->mapset = NULL;
    }
    return 0;
}
