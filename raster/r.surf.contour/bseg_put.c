#include <grass/gis.h>
#include "cseg.h"

int bseg_put(BSEG * bseg, CELL * value, int row, int col)
{
    unsigned char old_value;
    char errmsg[200];

    if (segment_get(&(bseg->seg), (int *)&old_value, row, col >> 3) < 0) {
	sprintf(errmsg,
		"bseg_put(): could not read segment file at r:%d c:%d",
		(int)row, (int)col);
	G_warning(errmsg);
	return -1;
    }
    if (*value)
	old_value |= (1 << (col & 7));
    else
	old_value &= ~(1 << (col & 7));
    if (segment_put(&(bseg->seg), (int *)&old_value, row, col >> 3) < 0) {
	sprintf(errmsg,
		"bseg_put(): could not write segment file at r:%d c:%d",
		(int)row, (int)col);
	G_warning(errmsg);
	return -2;
    }
    return 0;
}
