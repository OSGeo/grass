#include <grass/gis.h>
#include <grass/raster.h>
#include <unistd.h>
#include <grass/segment.h>
#include "cseg.h"

int seg_close(SSEG * sseg)
{
    segment_release(&(sseg->seg));
    close(sseg->fd);
    unlink(sseg->filename);
    return 0;
}
