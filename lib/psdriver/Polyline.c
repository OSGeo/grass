
#include "psdriver.h"

void PS_Polyline_abs(const int *xarray, const int *yarray, int number)
{
    int i;

    if (number < 2)
	return;

    output("%d %d POLYLINESTART\n", xarray[0], yarray[0]);

    for (i = 1; i < number; i++)
	output("%d %d POLYLINEVERTEX\n", xarray[i], yarray[i]);

    output("POLYLINEEND\n");
}
