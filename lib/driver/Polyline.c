#include "driver.h"
#include "driverlib.h"

void COM_Polyline_abs(const int *xarray, const int *yarray, int number)
{
    int i;

    if (driver->Polyline_abs) {
	(*driver->Polyline_abs) (xarray, yarray, number);
	return;
    }

    COM_Move_abs(xarray[0], yarray[0]);

    for (i = 1; i < number; i++)
	COM_Cont_abs(xarray[i], yarray[i]);
}

void COM_Polyline_rel(const int *xarray, const int *yarray, int number)
{
    int i;

    if (driver->Polyline_rel) {
	(*driver->Polyline_rel) (xarray, yarray, number);
	return;
    }

    COM_Move_rel(xarray[0], yarray[0]);

    for (i = 1; i < number; i++)
	COM_Cont_rel(xarray[i], yarray[i]);
}
