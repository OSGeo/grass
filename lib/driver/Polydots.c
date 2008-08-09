#include "driver.h"
#include "driverlib.h"

void COM_Polydots_abs(const double *xarray, const double *yarray, int number)
{
    int i;

    if (driver->Polydots) {
	(*driver->Polydots) (xarray, yarray, number);
	return;
    }

    for (i = 0; i < number; i++) {
	COM_Move_abs(xarray[i], yarray[i]);
	COM_Cont_rel(0, 0);
    }
}

void COM_Polydots_rel(const double *xarray, const double *yarray, int number)
{
    int i;

    for (i = 0; i < number; i++) {
	COM_Move_rel(xarray[i], yarray[i]);
	COM_Cont_rel(0, 0);
    }
}
