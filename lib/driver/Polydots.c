#include "driver.h"
#include "driverlib.h"

void COM_Polydots_abs(const double *xarray, const double *yarray, int number)
{
    int i;

    for (i = 0; i < number; i++)
	COM_Point(xarray[i], yarray[i]);
}

