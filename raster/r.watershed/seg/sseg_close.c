#include <grass/gis.h>
#include <unistd.h>
#include <grass/segment.h>
#include "Gwater.h"

int seg_close(SSEG * sseg)
{
    Segment_close(&(sseg->seg));

    return 0;
}
