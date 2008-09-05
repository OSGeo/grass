#include <stdlib.h>
#include <grass/gis.h>
#include "driver.h"
#include "driverlib.h"

void COM_Polygon_abs(const double *xarray, const double *yarray, int number)
{
    if (driver->Polygon)
	(*driver->Polygon) (xarray, yarray, number);
}

