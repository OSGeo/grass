
#include "psdriver.h"

void PS_Polygon(const double *xarray, const double *yarray, int number)
{
    int i;

    if (number < 2)
	return;

    output("%f %f POLYGONSTART\n", xarray[0], yarray[0]);

    for (i = 1; i < number; i++)
	output("%f %f POLYGONVERTEX\n", xarray[i], yarray[i]);

    output("POLYGONEND\n");
}
