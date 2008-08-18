#include <stdlib.h>
#include <grass/gis.h>
#include "driver.h"
#include "driverlib.h"

void COM_Polygon_abs(const double *xarray, const double *yarray, int number)
{
    if (driver->Polygon)
	(*driver->Polygon) (xarray, yarray, number);
}

void COM_Polygon_rel(const double *xarray, const double *yarray, int number)
{
    static double *xa, *ya;
    static int nalloc;
    int i;

    if (number > nalloc) {
	nalloc = number;
	xa = G_realloc(xa, (size_t) nalloc * sizeof(double));
	ya = G_realloc(ya, (size_t) nalloc * sizeof(double));
    }

    xa[0] = xarray[0] + cur_x;
    ya[0] = yarray[0] + cur_y;

    for (i = 1; i < number; i++) {
	xa[i] = xa[i - 1] + xarray[i];
	ya[i] = ya[i - 1] + yarray[i];
    }

    COM_Polygon_abs(xa, ya, number);
}
