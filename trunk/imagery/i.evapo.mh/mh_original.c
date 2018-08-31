#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /*Hargreaves et al, 1985. */
double mh_original(double ra, double tavg, double tmax, double tmin, double p)
{
    double td, result;

    td = tmax - tmin;
    if (tavg > 100.0) {
	tavg = tavg - 273.15;	/*in case Temperature is in Kelvin */
    }
    ra = ra * (24.0 * 60.0 * 60.0 / 1000.0);	/*convert W -> MJ/d */
    result = 0.0023 * 0.408 * ra * (tavg + 17.8) * pow(td, 0.5);
    return result;
}
