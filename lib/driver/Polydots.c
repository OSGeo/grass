#include "driver.h"
#include "driverlib.h"

void COM_Polydots_abs(const int *xarray, const int *yarray, int number)
{
    int i;

    if (driver->Polydots_abs) {
	(*driver->Polydots_abs) (xarray, yarray, number);
	return;
    }

    for (i = 0; i < number; i++) {
	COM_Move_abs(xarray[i], yarray[i]);
	COM_Cont_rel(0, 0);
    }
}

void COM_Polydots_rel(const int *xarray, const int *yarray, int number)
{
    int i;

    if (driver->Polydots_rel) {
	(*driver->Polydots_rel) (xarray, yarray, number);
	return;
    }

    for (i = 0; i < number; i++) {
	COM_Move_rel(xarray[i], yarray[i]);
	COM_Cont_rel(0, 0);
    }
}
