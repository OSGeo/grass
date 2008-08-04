#include <grass/rowio.h>

void rowio_forget(ROWIO * R, int row)
{
    int i;

    if (row < 0)
	return;

    for (i = 0; i < R->nrows; i++)
	if (row == R->rcb[i].row) {
	    R->rcb[i].row = -1;	/* no longer in memory */
	    break;
	}
}
