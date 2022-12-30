#include <stdio.h>
#include <math.h>
#include <stdlib.h>
<<<<<<< HEAD
<<<<<<< HEAD
#include <grass/gis.h>

/*Hargreaves et al, 1985. */
double mh_original(double ra, double tavg, double tmax, double tmin,
                   double p UNUSED)
=======

/*Hargreaves et al, 1985. */
=======

/*Hargreaves et al, 1985. */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
double mh_original(double ra, double tavg, double tmax, double tmin, double p)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    double td, result;

    td = tmax - tmin;
    if (tavg > 100.0) {
        tavg = tavg - 273.15; /*in case Temperature is in Kelvin */
    }
    ra = ra * (24.0 * 60.0 * 60.0 / 1000.0); /*convert W -> MJ/d */
    result = 0.0023 * 0.408 * ra * (tavg + 17.8) * pow(td, 0.5);
    return result;
}
