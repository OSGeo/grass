#include <grass/gis.h>
#include <unistd.h>
#include <grass/segment.h>
#include "Gwater.h"

int cseg_close(CSEG * cseg)
{
    Segment_close(&(cseg->seg));
    if (cseg->name) {
	G_free(cseg->name);
	cseg->name = NULL;
    }
    if (cseg->subproject) {
	G_free(cseg->subproject);
	cseg->subproject = NULL;
    }
    return 0;
}
