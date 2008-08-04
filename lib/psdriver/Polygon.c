
#include "psdriver.h"

void PS_Polygon_abs(const int *xarray, const int *yarray, int number)
{
    int i;

    if (number < 2)
	return;

    output("%d %d POLYGONSTART\n", xarray[0], yarray[0]);

    for (i = 1; i < number; i++)
	output("%d %d POLYGONVERTEX\n", xarray[i], yarray[i]);

    output("POLYGONEND\n");
}
