#include <stdio.h>
#include <stdlib.h>
#include <grass/rowio.h>

void Rowio_release(ROWIO * R)
{
    int i;

    if (R->rcb) {
	for (i = 0; i < R->nrows && R->rcb[i].buf; i++)
	    free(R->rcb[i].buf);
	free(R->rcb);
	R->rcb = NULL;
    }
}
