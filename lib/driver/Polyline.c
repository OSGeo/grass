#include "driver.h"
#include "driverlib.h"

void COM_Polyline_abs(const double *xarray, const double *yarray, int number)
{
    int i;

    COM_Begin();
    COM_Move(xarray[0], yarray[0]);
    for (i = 1; i < number; i++)
	COM_Cont(xarray[i], yarray[i]);
    COM_Stroke();
}

