
#include "psdriver.h"

void PS_Polyline(const double *xarray, const double *yarray, int number)
{
    int i;

    if (number < 2)
	return;

    output("%f %f POLYLINESTART\n", xarray[0], yarray[0]);

    for (i = 1; i < number; i++)
	output("%f %f POLYLINEVERTEX\n", xarray[i], yarray[i]);

    output("POLYLINEEND\n");
}
