#include "driver.h"
#include "driverlib.h"

void COM_Polyline_abs(const double *xarray, const double *yarray, int number)
{
    int i;

    if (driver->Polyline) {
	(*driver->Polyline) (xarray, yarray, number);
	return;
    }

    COM_Move_abs(xarray[0], yarray[0]);

    for (i = 1; i < number; i++)
	COM_Cont_abs(xarray[i], yarray[i]);
}

void COM_Polyline_rel(const double *xarray, const double *yarray, int number)
{
    int i;

    COM_Move_rel(xarray[0], yarray[0]);

    for (i = 1; i < number; i++)
	COM_Cont_rel(xarray[i], yarray[i]);
}
