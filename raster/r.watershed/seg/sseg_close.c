#include <grass/gis.h>
#include <unistd.h>
#include <grass/segment.h>
#include "Gwater.h"

int seg_close(SSEG * sseg)
{
    segment_release(&(sseg->seg));
    close(sseg->fd);
    unlink(sseg->filename);
    return 0;
}
