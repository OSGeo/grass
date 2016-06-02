#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /*Droogers and Allen, 2001. */
    /*p is [mm/month] */
double mh_eto(double ra, double tavg, double tmax, double tmin, double p)
{
    double td, result;

    td = tmax - tmin;
    if (tavg > 100.0) {
	tavg = tavg - 273.15;	/*in case temperature is in Kelvin */
    }
    ra = ra * (24.0 * 60.0 * 60.0 / 1000.0);	/*convert W -> MJ/d */
    result =
	0.0013 * 0.408 * ra * (tavg + 17.0) * pow((td - 0.0123 * p), 0.76);
    return result;
}
