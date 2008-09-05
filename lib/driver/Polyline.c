#include "driver.h"
#include "driverlib.h"

void COM_Polyline_abs(const double *xarray, const double *yarray, int number)
{
    int i;

    if (driver->Polyline) {
	(*driver->Polyline) (xarray, yarray, number);
	return;
    }

    for (i = 1; i < number; i++)
	COM_Line_abs(xarray[i-1], yarray[i-1], xarray[i], yarray[i]);
}

