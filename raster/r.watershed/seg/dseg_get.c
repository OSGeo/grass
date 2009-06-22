#include <grass/gis.h>
#include <grass/segment.h>
#include "cseg.h"

int dseg_get(DSEG * dseg, double *value, int row, int col)
{
    if (segment_get(&(dseg->seg), (DCELL *) value, row, col) < 0) {
	G_warning("dseg_get(): could not read segment file");
	return -1;
    }
    return 0;
}
