#include <grass/gis.h>
#include <unistd.h>
#include "Gwater.h"

int bseg_close(BSEG * bseg)
{
    Segment_close(&(bseg->seg));
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
