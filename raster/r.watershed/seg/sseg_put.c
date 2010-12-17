#include <grass/gis.h>
#include <grass/segment.h>
#include "cseg.h"

int seg_put(SSEG * sseg, char *value, int row, int col)
{
    if (segment_put(&(sseg->seg), value, row, col) < 0) {
	G_warning("seg_put(): could not write segment file");
	return -1;
    }
    return 0;
}

int seg_put_row(SSEG * sseg, char *value, int row)
{
    if (segment_put_row(&(sseg->seg), value, row) < 0) {
	G_warning("seg_put(): could not write segment file");
	return -1;
    }
    return 0;
}
