#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /*Hargreaves-Samani, 1985. */
double mh_samani(double ra, double tavg, double tmax, double tmin)
{
    double td, result;

    td = tmax - tmin;
    if (tavg > 100.0) {
	tavg = tavg - 273.15;	/*in case Temperature is in Kelvin */
    }
    ra = ra * (24.0 * 60.0 * 60.0 / 1000.0);	/* convert W -> MJ/d */
    result =
	0.0023 * 0.408 * ra * pow(td,
				  0.5) * ((tmax + tmin) / 2 + 17.8) / 2.45;
    return result;
}
