#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <grass/gis.h>

#define PI M_PI

    /* Conversion of Radiance to Reflectance for ASTER */
double rad2ref_aster(double radiance, double doy, double sun_elevation,
                     double k_exo)
{
    double result, ds;

    ds = (1 + 0.01672 * sin(2 * PI * (doy - 93.5) / 365));
    result =
        (radiance /
         ((cos((90 - sun_elevation) * PI / 180) / (PI * ds * ds)) * k_exo));
    return result;
}
