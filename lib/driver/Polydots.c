#include "driver.h"
#include "driverlib.h"

void COM_Polydots_abs(const double *xarray, const double *yarray, int number)
{
    int i;

    if (driver->Polydots) {
	(*driver->Polydots) (xarray, yarray, number);
	return;
    }

    for (i = 0; i < number; i++)
	COM_Line_abs(xarray[i], yarray[i], xarray[i], yarray[i]);
}

